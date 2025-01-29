#include <algorithm>
#include <string>
#include <vector>

class	serverIRC {

	private:
		std::vector<int> clients;
		int port;
		std::string password;

	public:
		serverIRC();
		serverIRC(int port, std::string pass);
		~serverIRC();
		const int& getPort() const;
		bool checkPass(std::string inputPass);
		void start();
		void close();
		void addClient();
		void kick(std::string nClient);
		void invite(std::string nClient, int channel);
		void topic(); // Change or view the channel topic
		void mode(char mode); // Change the channel’s mode:
			//· i: Set/remove Invite-only channel
			//· t: Set/remove the restrictions of the TOPIC command to channel operators
			//· k: Set/remove the channel key (password)
			//· o: Give/take channel operator privilege
			//· l: Set/remove the user limit to channel



}
