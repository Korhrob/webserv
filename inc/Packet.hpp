#pragma once

enum PacketID
{
	Connect,
	Message,
	Disconnect
};

char*	createMessage(PacketID packet_id, const std::string& message);