#include <iostream>
#include "Spotify.h"

Spotify spotify;

DWORD WINAPI SpotifyAdsBlockerProc(__in LPVOID lpParameter)
{
	while (1)
	{
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
}
