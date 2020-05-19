#include <iostream>
#include "Spotify.h"

Spotify spotify;

DWORD WINAPI SpotifyAdsBlockerProc(__in LPVOID lpParameter)
{
	while (1)
	{
		try
		{
			if (spotify.IsAdsPlaying())
			{
				spotify.Mute();

				while (spotify.IsAdsPlaying())
				{
					Sleep(500);
				}

				spotify.Unmute();
			}

			Sleep(500);
		}
		catch (const char* msg)
		{
			printf("error %s\n", msg);
		}
	}

	return 0;
}

HANDLE StartSpotifyAdsBlocker()
{
	return CreateThread(NULL,
		0,
		SpotifyAdsBlockerProc,
		0,
		0,
		NULL);
}

int main()
{
	HANDLE spotifyThreadHandler;

    std::cout << "Starting spotify ad blocker!\n";

	spotifyThreadHandler = StartSpotifyAdsBlocker();
	WaitForSingleObject(spotifyThreadHandler, INFINITE);
}
