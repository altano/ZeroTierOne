/*
 * ZeroTier One - Network Virtualization Everywhere
 * Copyright (C) 2011-2015  ZeroTier, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ZT_SQLITENETWORKCONTROLLER_HPP
#define ZT_SQLITENETWORKCONTROLLER_HPP

#include <stdint.h>

#include <string>
#include <map>
#include <vector>
#include <set>

#include "../node/Constants.hpp"

#include "../node/NetworkController.hpp"
#include "../node/Mutex.hpp"
#include "../node/Utils.hpp"
#include "../node/InetAddress.hpp"

#include "../osdep/OSUtils.hpp"

#include "../ext/json/json.hpp"

// Expiration time for network member cache entries in ms
#define ZT_NETCONF_NETWORK_MEMBER_CACHE_EXPIRE 30000

namespace ZeroTier {

class Node;

class EmbeddedNetworkController : public NetworkController
{
public:
	EmbeddedNetworkController(Node *node,const char *dbPath);
	virtual ~EmbeddedNetworkController();

	virtual NetworkController::ResultCode doNetworkConfigRequest(
		const InetAddress &fromAddr,
		const Identity &signingId,
		const Identity &identity,
		uint64_t nwid,
		const Dictionary<ZT_NETWORKCONFIG_METADATA_DICT_CAPACITY> &metaData,
		NetworkConfig &nc);

	unsigned int handleControlPlaneHttpGET(
		const std::vector<std::string> &path,
		const std::map<std::string,std::string> &urlArgs,
		const std::map<std::string,std::string> &headers,
		const std::string &body,
		std::string &responseBody,
		std::string &responseContentType);
	unsigned int handleControlPlaneHttpPOST(
		const std::vector<std::string> &path,
		const std::map<std::string,std::string> &urlArgs,
		const std::map<std::string,std::string> &headers,
		const std::string &body,
		std::string &responseBody,
		std::string &responseContentType);
	unsigned int handleControlPlaneHttpDELETE(
		const std::vector<std::string> &path,
		const std::map<std::string,std::string> &urlArgs,
		const std::map<std::string,std::string> &headers,
		const std::string &body,
		std::string &responseBody,
		std::string &responseContentType);

private:
	static void _circuitTestCallback(ZT_Node *node,ZT_CircuitTest *test,const ZT_CircuitTestReport *report);

	// JSON blob I/O
	inline nlohmann::json _readJson(const std::string &path)
	{
		std::string buf;
		if (OSUtils::readFile(path.c_str(),buf)) {
			try {
				return nlohmann::json::parse(buf);
			} catch ( ... ) {}
		}
		return nlohmann::json::object();
	}
	inline bool _writeJson(const std::string &path,const nlohmann::json &obj)
	{
		return OSUtils::writeFile(path.c_str(),obj.dump(2));
	}

	// Network base path and network JSON path
	inline std::string _networkBP(const uint64_t nwid,bool create)
	{
		char tmp[64];
		std::string p(_path + ZT_PATH_SEPARATOR_S + "network");
		if (create) OSUtils::mkdir(p.c_str());
		p.push_back(ZT_PATH_SEPARATOR);
		Utils::snprintf(tmp,sizeof(tmp),"%.16llx",nwid);
		p.append(tmp);
		if (create) OSUtils::mkdir(p.c_str());
		return p;
	}
	inline std::string _networkJP(const uint64_t nwid,bool create)
	{
		return (_networkBP(nwid,create) + ZT_PATH_SEPARATOR + "config.json");
	}

	// Member base path and member JSON path
	inline std::string _memberBP(const uint64_t nwid,const Address &member,bool create)
	{
		std::string p(_networkBP(nwid,create));
		p.push_back(ZT_PATH_SEPARATOR);
		p.append("member");
		if (create) OSUtils::mkdir(p.c_str());
		p.push_back(ZT_PATH_SEPARATOR);
		p.append(member.toString());
		if (create) OSUtils::mkdir(p.c_str());
		return p;
	}
	inline std::string _memberJP(const uint64_t nwid,const Address &member,bool create)
	{
		return (_memberBP(nwid,member,create) + ZT_PATH_SEPARATOR + "config.json");
	}

	// We cache the members of networks in memory to avoid having to scan the filesystem so much
	std::map< uint64_t,std::pair< std::map< Address,nlohmann::json >,uint64_t > > _networkMemberCache;
	Mutex _networkMemberCache_m;

	// Gathers a bunch of statistics about members of a network, IP assignments, etc. that we need in various places
	// This does lock _networkMemberCache_m
	struct _NetworkMemberInfo
	{
		_NetworkMemberInfo() : authorizedMemberCount(0),activeMemberCount(0),totalMemberCount(0) {}
		std::set<Address> activeBridges;
		std::set<InetAddress> allocatedIps;
		unsigned long authorizedMemberCount;
		unsigned long activeMemberCount;
		unsigned long totalMemberCount;
	};
	void _getNetworkMemberInfo(uint64_t now,uint64_t nwid,_NetworkMemberInfo &nmi);

	inline void _addNetworkNonPersistedFields(nlohmann::json &network,uint64_t now,const _NetworkMemberInfo &nmi)
	{
		network["clock"] = now;
		network["authorizedMemberCount"] = nmi.authorizedMemberCount;
		network["activeMemberCount"] = nmi.activeMemberCount;
		network["totalMemberCount"] = nmi.totalMemberCount;
	}

	// These are const after construction
	Node *const _node;
	std::string _path;

	// Circuit tests outstanding
	struct _CircuitTestEntry
	{
		ZT_CircuitTest *test;
		std::string jsonResults;
	};
	std::map< uint64_t,_CircuitTestEntry > _circuitTests;
	Mutex _circuitTests_m;

	// Last request time by address, for rate limitation
	std::map< std::pair<uint64_t,uint64_t>,uint64_t > _lastRequestTime;
	Mutex _lastRequestTime_m;
};

} // namespace ZeroTier

#endif