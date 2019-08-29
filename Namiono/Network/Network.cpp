/*
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Namiono/Namiono.h>

namespace Namiono
{
	namespace Network
	{
		void Handle_Request(ServiceType type, Server* server, int iface, Client* client, Packet* packet)
		{
			services.at(type)->Handle_Service_Request(type, server, iface, client, packet);
		}

#ifdef _WIN32
		bool Network::Init_Winsock(_INT32 major, _INT32 minor)
		{
			WSADATA wsa;
			ClearBuffer(&wsa, sizeof wsa);

			return WSAStartup(MAKEWORD(major, minor), &wsa) == 0;
		}

		bool Network::Close_Winsock()
		{
			return WSACleanup() == 0;
		}
#endif
		Network::Network(const std::string& rootDir)
		{
			printf("[I] Starting network...\n");
			using namespace Namiono::Services;

			FILE* fil = fopen("Config/servers.txt", "r");

			if (fil != nullptr)
			{
				char line[1024];
				ClearBuffer(line, sizeof line);
				while (fgets(line, sizeof line, fil) != nullptr)
				{
					char desc[64];
					ClearBuffer(desc, 64);
					char addr[1024];
					ClearBuffer(addr, 1024);

					std::vector<_IPADDR> addrs;

					if (sscanf(line, "%s | %s", &addr, &desc) != 0)
					{
						std::string _addrline = std::string(addr);
						std::vector<std::string> _addrs = Functions::Split(_addrline, std::string(","));

						for (_SIZET i = 0; i < _addrs.size(); i++)
						{
							addrs.emplace_back(inet_addr(_addrs.at(i).c_str()));
						}
					}
					
					for (_SIZET i = 0; i < addrs.size(); i++)
					{
						Get_UpstreamServers()->emplace_back(addrs.at(i));
					}
				}

				fclose(fil);
			}

			services.emplace(DHCP_SERVER, new DHCP_Service(*Get_UpstreamServers()));
			services.emplace(BINL_SERVER, new ProxyDHCP_Service());
			services.emplace(TFTP_SERVER, new TFTP_Service(rootDir));

			servers.emplace_back(&addresses, Handle_Request);
		}


		Network::~Network()
		{
		}

		void Network::Init()
		{
#ifdef _WIN32
			if (!Init_Winsock(2, 2))
			{
				printf("[E] Error: %s\n", strerror(WSAGetLastError()));
				return;
			}
#endif

			for (_SIZET i = 0; i < servers.size(); i++)
			{
				servers.at(i).Init();
			}
		}

		void Network::Start()
		{
			for (_SIZET i = 0; i < servers.size(); i++)
			{
				servers.at(i).Start();
			}
		}

		void Network::Listen()
		{
			for (_SIZET i = 0; i < servers.size(); i++)
			{
				servers.at(i).Listen(&listenThreads);
			}

			for (_SIZET i = 0; i < listenThreads.size(); i++)
			{
				listenThreads.at(i).join();
			}
		}

		std::vector<BootServerEntry>* Network::Get_BootServers()
		{
			return &serverlist;
		}

		std::vector<_IPADDR>* Network::Get_UpstreamServers()
		{
			return &dhcpservers;
		}

		void Network::Close()
		{
			for (_SIZET i = 0; i < servers.size(); i++)
			{
				servers.at(i).Close();
			}

#ifdef _WIN32
			if (!Close_Winsock())
			{
				printf("[E] Closing Winsock - Error: %d\n", WSAGetLastError());
				return;
			}
#endif
		}
	}
}
