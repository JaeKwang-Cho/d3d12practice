#pragma once
#include <WinSock2.h>
#include <stdint.h>

#pragma comment(lib, "ws2_32")

// ============ Client 와 공유 ============ 
struct ScreenImageHeader
{
	uint32_t currPacketNumber;
	uint32_t totalPacketsNumber;
};

#define SERVER_PORT (7654) // Server는 4567
#define MAX_PACKET_SIZE (1200)
#define HEADER_SIZE sizeof(ScreenImageHeader)
#define DATA_SIZE (MAX_PACKET_SIZE - HEADER_SIZE)
#define THREAD_NUMBER_BY_FRAME (3)
// =======================================