#include "MessageCache.hpp"
#include "ProfileCache.hpp"
#include "Frontend.hpp"
#include "DiscordInstance.hpp"

constexpr int MESSAGES_PER_REQUEST = 50;

using nlohmann::json;
static MessageCache g_MCSingleton;

MessageCache::MessageCache()
{
}

void MessageCache::GetLoadedMessages(Snowflake channel, Snowflake guild, std::list<Message>& out)
{
	MessageChunkList& lst = m_mapMessages[channel];
	lst.m_guild = guild;

	for (auto& msg : lst.m_messages)
		out.push_back(msg.second);
}

void MessageCache::ProcessRequest(Snowflake channel, ScrollDir::eScrollDir sd, Snowflake anchor, nlohmann::json& j, const std::string& channelName)
{
	MessageChunkList& lst = m_mapMessages[channel];
	lst.ProcessRequest(sd, anchor, j, channelName);
}

void MessageCache::AddMessage(Snowflake channel, const Message& msg)
{
	m_mapMessages[channel].AddMessage(msg);
}

void MessageCache::EditMessage(Snowflake channel, const Message& msg)
{
	m_mapMessages[channel].EditMessage(msg);
}

void MessageCache::DeleteMessage(Snowflake channel, Snowflake msg)
{
	m_mapMessages[channel].DeleteMessage(msg);
}

int MessageCache::GetMentionCountSince(Snowflake channel, Snowflake message, Snowflake user)
{
	return m_mapMessages[channel].GetMentionCountSince(message, user);
}

void MessageCache::ClearAllChannels()
{
	m_mapMessages.clear();
}

Message* MessageCache::GetLoadedMessage(Snowflake channel, Snowflake message)
{
	return m_mapMessages[channel].GetLoadedMessage(message);
}

MessageCache* GetMessageCache()
{
	return &g_MCSingleton;
}

MessageChunkList::MessageChunkList()
{
	// Add a single gap message to fetch stuff
	Message msg;
	msg.m_type = MessageType::GAP_UP;
	msg.m_anchor = 0;
	msg.m_snowflake = 0;
	msg.m_author = GetFrontend()->GetPleaseWaitText();
	msg.m_message = "";
	msg.m_dateFull = "";
	msg.m_dateCompact = "";
	m_messages[msg.m_snowflake] = std::move(msg);
}

void MessageChunkList::ProcessRequest(ScrollDir::eScrollDir sd, Snowflake gap, json& j, const std::string& channelName)
{
	Snowflake lowestMsg = (Snowflake) -1LL, highestMsg = 0;

	bool addedMessages = false;

	// remove the anchor
	auto iter = m_messages.find(gap);
	if (iter != m_messages.end())
	{
		if (iter->second.IsLoadGap())
			m_messages.erase(iter);
	}

	// for each message
	int receivedMessages = 0;
	for (json& data : j)
	{
		Message msg;
		msg.Load(data, m_guild);

		if (!addedMessages)
		{
			if (m_messages.find(msg.m_snowflake) == m_messages.end())
				addedMessages = true;
		}

		m_messages[msg.m_snowflake] = std::move(msg);
		receivedMessages++;

		if (lowestMsg > msg.m_snowflake)
			lowestMsg = msg.m_snowflake;
		if (highestMsg < msg.m_snowflake)
			highestMsg = msg.m_snowflake;
	}

	bool addBefore = sd != ScrollDir::AFTER && receivedMessages >= MESSAGES_PER_REQUEST;
	bool addAfter  = sd != ScrollDir::BEFORE;

	Message msg;
	msg.m_author = "";
	msg.m_message = "";
	msg.m_dateFull = "";
	msg.m_dateCompact = "";

	if (receivedMessages < MESSAGES_PER_REQUEST)
	{
		msg.m_type = MessageType::CHANNEL_HEADER;
		msg.m_snowflake = 1;
		msg.m_author = channelName;
		m_messages[msg.m_snowflake] = std::move(msg);
	}

	msg.m_author = GetFrontend()->GetPleaseWaitText();
	if (addBefore && addedMessages)
	{
		msg.m_type = MessageType::GAP_UP;
		msg.m_anchor = lowestMsg;
		msg.m_snowflake = lowestMsg - 1;
		m_messages[msg.m_snowflake] = std::move(msg);
	}

	if (addAfter && addedMessages)
	{
		msg.m_type = MessageType::GAP_DOWN;
		msg.m_anchor = highestMsg;
		msg.m_snowflake = highestMsg + 1;
		m_messages[msg.m_snowflake] = std::move(msg);
	}

	GetDiscordInstance()->OnFetchedMessages(gap, sd);
}

void MessageChunkList::AddMessage(const Message& msg)
{
	if (msg.m_anchor)
		DeleteMessage(msg.m_anchor);

	m_messages[msg.m_snowflake] = msg;
}

void MessageChunkList::EditMessage(const Message& msg)
{
	DeleteMessage(msg.m_snowflake);
	m_messages[msg.m_snowflake] = msg;
}

void MessageChunkList::DeleteMessage(Snowflake message)
{
	auto iter = m_messages.find(message);
	if (m_messages.end() != iter)
		m_messages.erase(iter);
}

int MessageChunkList::GetMentionCountSince(Snowflake message, Snowflake user)
{
	int mentCount = 0;

	for (auto& msg : m_messages)
	{
		if (msg.first < message)
			continue;

		if (msg.second.CheckWasMentioned(user, m_guild))
			mentCount++;
	}

	return mentCount;
}

Message* MessageChunkList::GetLoadedMessage(Snowflake message)
{
	auto iter = m_messages.find(message);
	if (m_messages.end() == iter)
		return nullptr;

	return &iter->second;
}
