#define DISCORDPP_IMPLEMENTATION
#include "discordpp.h"

#include <nlohmann/json.hpp>
#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <chrono>
#include <cstdint>
#include <thread>
#include <cmath>
#include <cctype>


#pragma comment(lib, "winhttp.lib")

using json = nlohmann::json;

std::string UrlEncode(const std::string& value)
{
	std::ostringstream escaped;

	escaped.fill('0');
	escaped << std::hex;

	for (unsigned char c : value)
	{
		if (std::isalnum(c) ||
			c == '-' ||
			c == '_' ||
			c == '.' ||
			c == '~')
		{
			escaped << c;
		}
		else
		{
			escaped << '%' << std::setw(2) << static_cast<int>(c);
		}
	}

	return escaped.str();
}

std::string GetFoobarData()
{
	HINTERNET hSession = WinHttpOpen(
		L"FoobarRPC/1.0",
		WINHTTP_ACCESS_TYPE_NO_PROXY,
		nullptr,
		nullptr,
		0
	);

	if (!hSession)
	{
		return "";
	}

	HINTERNET hConnect = WinHttpConnect(
		hSession,
		L"localhost",
		8880,
		0
	);

	if (!hConnect)
	{
		WinHttpCloseHandle(hSession);
		return "";
	}

	HINTERNET hRequest = WinHttpOpenRequest(
		hConnect,
		L"GET",
		L"/api/query?player=true&trcolumns=%25artist%25,%25title%25,%25album%",
		nullptr,
		WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES,
		0
	);

	if (!hRequest)
	{
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return "";
	}

	bool success = WinHttpSendRequest(
		hRequest,
		WINHTTP_NO_ADDITIONAL_HEADERS,
		0,
		nullptr,
		0,
		0,
		0
	);

	if (success)
	{
		success = WinHttpReceiveResponse(
			hRequest,
			nullptr
		);
	}

	std::string result;

	if (success)
	{
		DWORD available = 0;

		while (WinHttpQueryDataAvailable(hRequest, &available) && available > 0)
		{
			std::string buffer(available, '\0');

			DWORD downloaded = 0;

			if (!WinHttpReadData(
				hRequest,
				buffer.data(),
				available,
				&downloaded))
			{
				break;
			}

			result.append(buffer, 0, downloaded);
		}
	}

	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);

	return result;
}

std::string GetMusicBrainzReleaseId(
	const std::string& artist,
	const std::string& album)
{
	if (artist.empty() || album.empty())
	{
		return "";
	}

	std::string encodedArtist = UrlEncode(artist);
	std::string encodedAlbum = UrlEncode(album);

	std::wstring path =
		L"/ws/2/release/?query=artist:" +
		std::wstring(encodedArtist.begin(), encodedArtist.end()) +
		L"%20AND%20release:" +
		std::wstring(encodedAlbum.begin(), encodedAlbum.end()) +
		L"&fmt=json&limit=1";

	HINTERNET hSession = WinHttpOpen(
		L"FoobarRPC/1.0 (MusicBrainz lookup)",
		WINHTTP_ACCESS_TYPE_NO_PROXY,
		nullptr,
		nullptr,
		0
	);

	if (!hSession)
	{
		return "";
	}

	HINTERNET hConnect = WinHttpConnect(
		hSession,
		L"musicbrainz.org",
		INTERNET_DEFAULT_HTTPS_PORT,
		0
	);

	if (!hConnect)
	{
		WinHttpCloseHandle(hSession);
		return "";
	}

	HINTERNET hRequest = WinHttpOpenRequest(
		hConnect,
		L"GET",
		path.c_str(),
		nullptr,
		WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES,
		WINHTTP_FLAG_SECURE
	);

	if (!hRequest)
	{
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return "";
	}

	// MusicBrainz vaatii järkevän User-Agentin
	std::wstring headers =
		L"User-Agent: FoobarDiscordRPC/1.0 (pekepaa@hotmail.com)\r\n"
		L"Accept: application/json\r\n";

	bool success = WinHttpSendRequest(
		hRequest,
		headers.c_str(),
		-1L,
		nullptr,
		0,
		0,
		0
	);

	if (success)
	{
		success = WinHttpReceiveResponse(
			hRequest,
			nullptr
		);
	}

	std::string result;

	if (success)
	{
		DWORD available = 0;

		while (WinHttpQueryDataAvailable(hRequest, &available)
			&& available > 0)
		{
			std::string buffer(available, '\0');

			DWORD downloaded = 0;

			if (!WinHttpReadData(
				hRequest,
				buffer.data(),
				available,
				&downloaded))
			{
				break;
			}

			result.append(buffer, 0, downloaded);
		}
	}


	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);

	if (result.empty())
	{
		return "";
	}

	try
	{
		json root = json::parse(result);

		if (!root.contains("releases") ||
			!root["releases"].is_array() ||
			root["releases"].empty())
		{
			std::cout << "MusicBrainz: no release found for "
				<< artist << " - " << album << std::endl;

			return "";
		}

		auto& release = root["releases"][0];

		if (!release.contains("id"))
		{
			return "";
		}

		std::string releaseId =
			release["id"].get<std::string>();

		std::cout << "MusicBrainz release ID: "
			<< releaseId << std::endl;

		return releaseId;
	}
	catch (const std::exception& e)
	{
		std::cerr << "MusicBrainz JSON error: "
			<< e.what() << std::endl;

		return "";
	}
}

std::string GetCoverArtUrl(const std::string& releaseId)
{
	if (releaseId.empty())
	{
		return "";
	}

	// Cover Art Archive tarjoaa suoraan kuvan release MBID:llä
	return "https://coverartarchive.org/release/" +
		releaseId +
		"/front-500";
}

int main()
{
	std::cout << "Getting foobar2000 data..." << std::endl;

	std::string data = GetFoobarData();

	auto client = std::make_shared<discordpp::Client>();

	client->SetApplicationId(1537005939868041218);

	std::string lastArtist;
	std::string lastTitle;
	std::string lastPlaybackState;
	std::string currentCoverUrl;
	double lastPosition = -1;

	while (true)
	{
		// Anna Discord Social SDK:n käsitellä callbackit
		discordpp::RunCallbacks();

		// Hae foobar2000:n tämänhetkiset tiedot
		std::string data = GetFoobarData();

		try
		{
			json root = json::parse(data);

			auto& player = root["player"];
			auto& item = player["activeItem"];

			std::string playbackState = player["playbackState"];

			std::string artist = "";
			std::string title = "";
			std::string album = "";

			double position = 0;
			double duration = 0;

			if (player.contains("activeItem") &&
				!player["activeItem"].is_null())
			{
				auto& item = player["activeItem"];

				if (item.contains("columns") &&
					item["columns"].is_array() &&
					item["columns"].size() >= 3)
				{
					if (!item["columns"][0].is_null())
						artist = item["columns"][0].get<std::string>();

					if (!item["columns"][1].is_null())
						title = item["columns"][1].get<std::string>();

					if (!item["columns"][2].is_null())
						album = item["columns"][2].get<std::string>();
				}

				if (item.contains("position") &&
					!item["position"].is_null())
				{
					position = item["position"].get<double>();
				}

				if (item.contains("duration") &&
					!item["duration"].is_null())
				{
					duration = item["duration"].get<double>();
				}
			}

			if (artist != lastArtist || title != lastTitle)
			{
				std::string releaseId =
					GetMusicBrainzReleaseId(artist, album);

				if (!releaseId.empty())
				{
					std::cout << "Release ID: "
						<< releaseId << std::endl;

					currentCoverUrl =
						GetCoverArtUrl(releaseId);

					std::cout << "Cover URL: "
						<< currentCoverUrl << std::endl;
				}
				else
				{
					currentCoverUrl = "";
				}
			}

			if (artist != lastArtist ||
				title != lastTitle ||
				playbackState != lastPlaybackState ||
				std::abs(position - lastPosition) > 2.0)
			{

				std::cout << "New song:" << std::endl;
				std::cout << "Artist: " << artist << std::endl;
				std::cout << "Title:  " << title << std::endl;

				auto now = std::chrono::system_clock::now();
				auto nowSeconds = std::chrono::duration_cast<std::chrono::seconds>(
					now.time_since_epoch()
				).count();

				int64_t startTimestamp =
					nowSeconds - static_cast<int64_t>(position);
				int64_t endTimestamp =
					startTimestamp + static_cast<int64_t>(duration);

				// Discord Activity
				discordpp::Activity activity;

				discordpp::ActivityAssets assets;

				if (!currentCoverUrl.empty())
				{
					assets.SetLargeImage(currentCoverUrl);
					assets.SetLargeText(album);
				}

				activity.SetAssets(assets);

				activity.SetType(discordpp::ActivityTypes::Listening);
				activity.SetDetails(artist + " - " + title);
				activity.SetState(artist);
				activity.SetState(playbackState == "playing" ? "Playing" : "Paused");

				discordpp::ActivityTimestamps timestamps;

				timestamps.SetStart(startTimestamp);
				timestamps.SetEnd(endTimestamp);

				activity.SetTimestamps(timestamps);
				activity.SetStatusDisplayType(
					discordpp::StatusDisplayTypes::Details);

				if (playbackState == "stopped")
				{
					activity.SetDetails("Foobar2000");
					activity.SetState("Stopped");

					discordpp::ActivityTimestamps timestamps;

					timestamps.SetStart(0);
					timestamps.SetEnd(0);

					activity.SetTimestamps(timestamps);
				}
				else if (playbackState == "paused")
				{

					int pausedMinutes = static_cast<int>(position) / 60;
					int pausedSeconds = static_cast<int>(position) % 60;

					int totalMinutes = static_cast<int>(duration) / 60;
					int totalSeconds = static_cast<int>(duration) % 60;

					std::string progress =
						"Paused: " +
						std::to_string(pausedMinutes) + ":" +
						(pausedSeconds < 10 ? "0" : "") +
						std::to_string(pausedSeconds) +
						" / " +
						std::to_string(totalMinutes) + ":" +
						(totalSeconds < 10 ? "0" : "") +
						std::to_string(totalSeconds);

					activity.SetState(progress);

					discordpp::ActivityTimestamps timestamps;

					timestamps.SetStart(0);
					timestamps.SetEnd(0);

					activity.SetTimestamps(timestamps);
				}

				else {
					activity.SetState("Playing");

					auto now = std::chrono::system_clock::now();

					auto nowSeconds =
						std::chrono::duration_cast<std::chrono::seconds>(
							now.time_since_epoch()
						).count();

					int64_t startTimestamp =
						nowSeconds - static_cast<int64_t>(position);

					int64_t endTimestamp =
						startTimestamp + static_cast<int64_t>(duration);

					discordpp::ActivityTimestamps timestamps;

					timestamps.SetStart(startTimestamp);
					timestamps.SetEnd(endTimestamp);

					activity.SetTimestamps(timestamps);


				}

				client->UpdateRichPresence(
					activity,
					[](const discordpp::ClientResult& result)
					{
						if (result.Successful())
						{
							std::cout << "Discord Rich Presence updated!"
								<< std::endl;
						}
						else
						{
							std::cout << "Discord Rich Presence update failed!"
								<< std::endl;
						}
					}
				);
			}
			lastArtist = artist;
			lastTitle = title;
			lastPlaybackState = playbackState;
			lastPosition = position;
		}

		catch (const std::exception& e)
		{
			std::cerr << "JSON error: " << e.what() << std::endl;
		}
		// Odotetaan yksi sekunti ennen seuraavaa tarkistusta
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	std::cout << std::endl;
	std::cout << "Press Enter to exit..." << std::endl;

	std::cin.get();

	return 0;
}