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

void StartSpotifyAdsBlocker()
{
	CreateThread(NULL,
		0,
		SpotifyAdsBlockerProc,
		0,
		0,
		NULL);
}

int main()
{
    std::cout << "Starting spotify ad blocker!\n";
	StartSpotifyAdsBlocker();
	while (1)
	{
	}
}
