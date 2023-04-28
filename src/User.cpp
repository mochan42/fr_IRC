#include "../inc/User.hpp"

//		*!* CONSTRUCTORS and DESTRUCTOR  *!*
//		------------------------------------

User::User(int fd, std::string ip, Server *ircserver)
{
	this->_server = ircserver;
	this->_userFd = fd;
	this->_ip = ip;
	this->_userName = "";
	this->_nickName = "";
	this->_realName = "";
	this->_channelList = std::vector<Channel*>();
	this->_isRegistered = false;
	this->_usernameSet = false;
	this->_welcomeMes = false;
	std::cout << "User with fd = " << this->getFd() << " connected with server." << std::endl;
}

User&		User::operator=(User &src)
{
	this->_userFd = src.getFd();
	this->_userName = src.getUserName();
	this->_nickName = src.getNickName();
	this->_realName = src.getRealName();
	this->_channelList = src._channelList;
	this->_isRegistered = src._isRegistered;
	this->_usernameSet = src._usernameSet;
	return (*this);
}

User::~User()
{
	std::cout << "User " << this->getUserName() << " removed from server." << std::endl;
}

//		*!* MSG transmission and Command execution *!*
//		----------------------------------------------

/**
 * @brief 
 * function to send a message 'msg' to the client of this user.
 *
 * @param msg std::string
 */
int			User::sendMsgToOwnClient(std::string msg)
{
	std::string str = msg + "\n";
	try
	{
		if (send(this->_userFd, str.c_str(), str.length(), 0) < 0)
			throw SendToOwnCLientException();
	}
	catch(const std::exception & e)
	{
		std::cerr << e.what() << '\n';
	}
	return (0);
}


/**
 * @brief 
 * function to send a message 'msg' to the user client with fd 'targetUserFd'.
 *
 * @param msg std::string
 * @param targetUserFd int
 */
int			User::sendMsgToTargetClient(std::string msg, int targetUserFd)
{
	std::string str = msg + "\n";
	try
	{
		if (send(targetUserFd, str.c_str(), str.length(), 0) < 0)
			throw SendToTargetCLientException();
	}
	catch(const std::exception & e)
	{
		std::cerr << e.what() << '\n';
	}
	return (0);
}

int	User::getPort(void)
{
	return(this->_userPort);
}

void	User::setPort(int setUserPort)
{
	this->_userPort = setUserPort;
}

//		*!* Command execution  *!*
//		---------------------------


/**
 * @brief 
 * function to call the received command.
 *
 * @param command std::string
 * @param args std::vector < std::string >
 */
void		User::executeCommand(std::string command, std::vector<std::string>& args)
{
	for (size_t i = 0; i < command.length(); i++)
		command[i] = std::toupper(command[i]);
	try
	{
		if (!this->_isRegistered && command == "PASS")
			registerUser(args);
		else if (!this->_isRegistered)
			throw notVerified();
		else
		{
			//if (_isRegistered == false && (command == "PRIVMSG" || command == "REAL" || command == "JOIN" || command == "MODE" || command == "WHO" || command == "INVITE" || command == "KICK" || command == "PART" || command == "NOTICE"))
			std::cout << "User::executeCommand called with command = " << command <<  std::endl;
			if (command == "CAP")
			{}
			else if (command == "USER")
				setUserName(args);
			else if (command == "PASS")
				registerUser(args);
			else if (command == "NICK")
				setNickName(args);
			else if (!_isRegistered || !_usernameSet)
				throw (notRegistered());
			else if (command == "REAL")
				setRealName(args);
			else if (command == "JOIN")
				joinChannel(args);
			else if (command == "MODE")
				mode(args);
			else if (command == "WHO")
				who(args);		
			else if (command == "TOPIC")
				changeTopic(args);
			else if (command == "INVITE")
				inviteUser(args);
			else if (command == "KICK")
				kickUser(args);
			else if (command == "PART")
				leaveChannel(args);
			else if (command == "NOTICE")
				sendNotification(args);
			else if (command == "PRIVMSG")
			{
				if (args[0].at(0) == '#')
					sendChannelMsg(args);
				else
					sendPrivateMsg(args);
			}
			else if (command == "QUIT")
				quitServer(args);
			else
				throw (commandNotFound());
		}
	}
	catch(notRegistered& e)
	{
		(void)e;
		sendMsgToOwnClient(RPY_ERR451_notRegistered());
	}
	catch(commandNotFound& e)
	{
		(void)e;
		sendMsgToOwnClient(RPY_ERR_commandNotfound(command));
	}
	catch(notVerified& e)
	{
		sendMsgToOwnClient(e.what());
	}

}

//		*!* NAME and ID Handling  *!*
//		-----------------------------

int			User::getFd(void)
{
	return (this->_userFd);
}

std::string		User::getIP(void)
{
	return (this->_ip);
}

void		User::registerUser(const std::vector<std::string>& args)
{
	std::cout << "User::checkServerPw called." << std::endl;

	try
	{
		if (args.empty())
			return;
		if (this->_server->verifyPassword(args[0]))
		{
			_isRegistered = true;
			sendMsgToOwnClient(RPY_pass(true));
			std::cout << "The user is now registered" << std::endl;
		}
		else
		{
			std::cout << "PW wrong: The User is not registered" << std::endl;
			throw (wrongPassword());
		}
	
	}
	catch(wrongPassword& e)
	{
		(void)e;
		sendMsgToOwnClient(RPY_pass(false));
	}
}

void		User::setNickName(const std::vector<std::string>& args)
{
	if (args.empty())
			return;
	std::string oldNick = _nickName;
	std::string newNick = args[0];

	try
	{
		if (_server->getUser(newNick))
			throw (nickInUse());
		if (!_welcomeMes)
		{
			_nickName = newNick;
			sendMsgToOwnClient(RPY_welcomeToServer());
			_welcomeMes = true;
			return;
		}
		_nickName = newNick;
		//create a list with all users from all channels:
		if (!_channelList.empty())
		{
			std::vector<User *> newUserList;
			std::vector<Channel *>::iterator channelIt = _channelList.begin();
			std::list<User *> *userList;
			for (; channelIt != _channelList.end(); ++channelIt)
			{
				userList = (*channelIt)->getListPtrOperators();
				for(std::list<User *>::iterator it = userList->begin(); it != userList->end(); ++it)
				{
					if (isUserInList(newUserList.begin(), newUserList.end(), *it) == false)
					{
						newUserList.push_back(*it);
					}
				}
				userList = (*channelIt)->getListPtrOrdinaryUsers();
				for(std::list<User *>::iterator it = userList->begin(); it != userList->end(); ++it)
				{
					if (isUserInList(newUserList.begin(), newUserList.end(), *it) == false)
					{
						newUserList.push_back(*it);
					}
				}
			}
			//Send all Users the message
			for (std::vector<User *>::iterator it = newUserList.begin(); it != newUserList.end(); ++it)
			{
				(*it)->sendMsgToOwnClient(RPY_newNick(oldNick));
			}
		}
		else
			sendMsgToOwnClient(RPY_newNick(oldNick));
		std::cout << "User::setNickname called. The _nickName of fd " << this->getFd() << " is now:  " << this->getNickName() << std::endl;
	}
	catch (nickInUse &e)
	{
		(void)e;
		sendMsgToOwnClient(RPY_ERR433_nickInUse(newNick));
	}
}

std::string	User::getNickName(void)
{
	return (this->_nickName);
}

void		User::setUserName(std::vector<std::string>& args)
{

	if (_usernameSet == true)
	{
		sendMsgToOwnClient(RPY_ERR462_alreadyRegistered());
	}
	else
	{
		if (args.size() != 4)
		{
			sendMsgToOwnClient("Wrong use of user cmd: <Username> 0 * :<RealName>");
			return;
		}
		_userName = args[0];
		_realName = argsToString(args.begin() + 3, args.end());
		_usernameSet = true;
	}
	std::cout << "User::setUserName called. The _UserName is now:  " << this->getUserName() << std::endl;
}

std::string		User::getUserName(void)
{
	return (this->_userName);
}

void		User::setRealName(const std::vector<std::string>& args)
{
	_realName = args[0];
}

std::string		User::getRealName(void)
{
	return (this->_realName);
}


//		*!* CHANNEL interacting  *!*
//		----------------------------


void		User::who(std::vector<std::string>& args)
{
	if (args.empty())
			return;
	if (args[0][0] == '#') //handle channel
	{
		std::string channel = args[0];
		Channel *channelPtr = _server->getChannel(channel);
		if (channelPtr)
		{
			std::list<User *> *opList = channelPtr->getListPtrOperators();
			std::list<User *> *ordinaryList = channelPtr->getListPtrOrdinaryUsers();
			std::list<User *>::iterator it = opList->begin();

			while (it != opList->end())
			{
				sendMsgToOwnClient(RPY_352_whoUser(*it, channel, true));						
				++it;
			}
			it = ordinaryList->begin();
			while (it != ordinaryList->end())
			{
				sendMsgToOwnClient(RPY_352_whoUser(*it, channel, false));
				++it;
			}
		}
		sendMsgToOwnClient(RPY_315_endWhoList(channel));
	}
	else //handle single user
	{
		std::string user = args[0];
		User *userptr = _server->getUser(user);

		if (userptr)
			sendMsgToOwnClient(userptr->RPY_352_whoUser(userptr, "*", false));
		sendMsgToOwnClient(RPY_315_endWhoList(user));
	}
}

void		User::changeTopic(std::vector<std::string>& args)
{
	std::string channel = args[0];
	Channel *chnptr = _server->getChannel(channel);
	if (!chnptr)
	{
		sendMsgToOwnClient(RPY_ERR401_noSuchNickChannel(channel));
		return;
	}
	if (args.size() == 1)
	{
		sendMsgToOwnClient(RPY_332_channelTopic(channel, chnptr->getTopic()));
		return;
	}
	if (chnptr->isModeSet(CHN_MODE_AdminSetTopic, CHN_OPT_CTRL_NotExclusive))
	{
		if (!chnptr->isUserInList(chnptr->getListPtrOperators(), this))
		{
			sendMsgToOwnClient(RPY_ERR482_notChannelOp(channel));
			return;
		}
	}
	chnptr->setTopic(args[1]);
	chnptr->broadcastMsg(RPY_newTopic(channel, args[1]), std::make_pair(false, (User *) NULL));
}


void 		User::inviteUser(std::vector<std::string>& args)
{
	if (args.size() < 2)
	{
		sendMsgToOwnClient("Syntax error: INVITE user #channel\n");
		return;
	}
	std::string nick = args[0];
	std::string channel = args[1];
	try
	{
		User *user = _server->getUser(nick);
		Channel	*chptr = _server->getChannel(channel);
		if (!user)
			throw (noSuchNick());
		else if (!chptr)
			throw (noSuchChannel());
		else
		{
			if (chptr->isUserInChannel(nick) != NULL)
				sendMsgToOwnClient(RPY_ERR443_alreadyOnChannel(nick, channel));
			else
			{
				chptr->updateUserList(chptr->getListPtrInvitedUsers(), user, USR_ADD);
				sendMsgToOwnClient(RPY_341_userAddedtoInviteList(nick, channel));
				//!!!     write to other person:   !!!!
				user->sendMsgToOwnClient(RPY_inviteMessage(nick, channel));
				//check if that works later
			}
		}
	}
	catch(noSuchChannel& e)
	{
		(void)e;
		sendMsgToOwnClient(RPY_ERR401_noSuchNickChannel(channel));
	}
	catch(noSuchNick& e)
	{
		(void)e;
		sendMsgToOwnClient(RPY_ERR401_noSuchNickChannel(nick));
	}
}


/**
 * @brief
 * function to add a user to a channel
 * or create a new channel and add the user to it
 * @param args All arguments after the cmd
 */
void		User::joinChannel(std::vector<std::string>& args)
{
	std::cout << "USER::joinChannel called." << std::endl;
	try
	{
		if (args[0][0] != '#')
			throw (badChannelMask());
		Channel *chptr = _server->getChannel(args[0]);
		if (chptr == NULL) //Create channel
		{

			std::cout << "Channel doesn't exists. Server::createChannel called." << std::endl;
			chptr = _server->createChannel(args[0], this);
			if (!chptr)
			{
				std::cerr << "Error while creating channel"<< std::endl;
				return;
			}
			chptr->broadcastMsg(RPY_joinChannelBroadcast(chptr, true), std::make_pair(false, (User *) NULL));
			sendMsgToOwnClient(RPY_createChannel(chptr));
			sendMsgToOwnClient(RPY_353_joinWho(chptr));
			sendMsgToOwnClient(RPY_315_endWhoList(chptr->getChannelName()));
			_channelList.push_back(chptr);
		}
		else //join channel
		{
			if (chptr->isModeSet(CHN_MODE_Protected, CHN_OPT_CTRL_NotExclusive))
			{
				if (args.size() <= 1 || !chptr->checkPassword(args[1]))
					throw (cannotJoinChannelPW());
			}
			if (chptr->isModeSet(CHN_MODE_Invite, CHN_OPT_CTRL_NotExclusive))
			{
				if (!chptr->isUserInList(chptr->getListPtrInvitedUsers(), this))
					throw (cannotJoinChannelIn());
				else
					chptr->updateUserList(chptr->getListPtrInvitedUsers(), this, USR_REMOVE);
			}
			if (chptr->getChannelCapacity() <= chptr->getNbrofActiveUsers())
				throw (channelCapacity());
			chptr->updateUserList(chptr->getListPtrOrdinaryUsers(), this, USR_ADD);
			sendMsgToOwnClient(RPY_joinChannel(chptr));
			chptr->broadcastMsg(RPY_joinChannelBroadcast(chptr, false), std::make_pair(false, (User *) NULL));
			sendMsgToOwnClient(RPY_353_joinWho(chptr));	
			sendMsgToOwnClient(RPY_315_endWhoList(chptr->getChannelName()));
			_channelList.push_back(chptr);
		}
	}
	catch (badChannelMask &e)
	{
		(void)e;
		sendMsgToOwnClient(RPY_ERR476_badChannelMask(args[0]));
	}
	catch (cannotJoinChannelPW &e)
	{
		(void)e;
		sendMsgToOwnClient(RPY_ERR475_canNotJoinK(args[0]));
	}
	catch (cannotJoinChannelIn &e)
	{
		(void)e;
		sendMsgToOwnClient(RPY_ERR473_canNotJoinI(args[0]));
	}
	catch (channelCapacity &e)
	{
		(void)e;
		sendMsgToOwnClient(RPY_ERR471_canNotJoinL(args[0]));
	}
}

void		User::removeChannelFromList(Channel* channel)
{
	std::vector<Channel *>::iterator iter = _channelList.begin(); 
	while (iter != _channelList.end())
	{
		if (channel->getChannelName() == (*iter)->getChannelName())
			iter = _channelList.erase(iter);
		else
			++iter;
	}
}


/**
 * @brief 
 * Will kick a user from a channel and inform all other user on the channel about it.
 * @param args All arguments after the cmd
 */
void		User::kickUser(std::vector<std::string>& args)
{
	if (args.size() < 2)
	{
		sendMsgToOwnClient("Syntax error: KICK #channel user :msg\n");
		return;
	}
	std::string channel = args[0];
	std::string nick = args[1];
	std::string	reason = "";
	
	if (args.size() > 2)
		reason = argsToString(args.begin() + 2, args.end());
	Channel *channelPtr;
	User	*tmpUser;
	
	try
	{
		channelPtr = _server->getChannel(channel);
		if (!channelPtr->isUserInList(channelPtr->getListPtrOperators(), this))
			throw (notAnOperator());

		if ((tmpUser = channelPtr->isUserInChannel(nick)))
		{
			channelPtr->broadcastMsg(RPY_kickedMessage(nick, channel, reason), std::make_pair(false, (User *) NULL));
			if (channelPtr->isUserInList(channelPtr->getListPtrOperators(), tmpUser))
				channelPtr->updateUserList(channelPtr->getListPtrOperators(), tmpUser, USR_REMOVE);
			if (channelPtr->isUserInList(channelPtr->getListPtrOrdinaryUsers(), tmpUser))
				channelPtr->updateUserList(channelPtr->getListPtrOrdinaryUsers(), tmpUser, USR_REMOVE);
			tmpUser->removeChannelFromList(channelPtr);
			if (channelPtr->isUserListEmpty(channelPtr->getListPtrOperators()) && channelPtr->isUserListEmpty(channelPtr->getListPtrOrdinaryUsers()))
				_server->remChannel(channelPtr->getChannelName());
		}
		else
			throw (notOnTheChannel());
	}
	catch (notAnOperator &e)
	{
		(void)e;
		sendMsgToOwnClient(RPY_ERR482_notChannelOp(channel));
	}
	catch (notOnTheChannel &e)
	{
		(void)e;
		sendMsgToOwnClient(RPY_ERR441_kickNotOnChannel(nick, channel));
	}
}

/**
 * @brief 
 * Function to leave a channel and inform all user in the channel about it.
 * @param args All arguments after the cmd
 */
void		User::leaveChannel(std::vector<std::string>& args)
{
	std::string	channel = args[0];
	Channel		*chPtr;

	try
	{
		
		if (!(chPtr = _server->getChannel(channel)))
			throw(noSuchChannel());
		if (chPtr->isUserInList(chPtr->getListPtrOrdinaryUsers(), this))
		{
			chPtr->broadcastMsg(RPY_leaveChannel(channel), std::make_pair(false, (User *) NULL));
			chPtr->updateUserList(chPtr->getListPtrOrdinaryUsers(), this, USR_REMOVE);
			removeChannelFromList(chPtr);
		}
		else if (chPtr->isUserInList(chPtr->getListPtrOperators(), this))
		{
			chPtr->broadcastMsg(RPY_leaveChannel(channel), std::make_pair(false, (User *) NULL));
			chPtr->updateUserList(chPtr->getListPtrOperators(), this, USR_REMOVE);
			removeChannelFromList(chPtr);
		}
		else
			throw(notOnTheChannel());
		if (chPtr->isUserListEmpty(chPtr->getListPtrOperators()) && chPtr->isUserListEmpty(chPtr->getListPtrOrdinaryUsers()))
			_server->remChannel(chPtr->getChannelName());

				
	}
	catch(noSuchChannel &e)
	{
		(void)e;
		sendMsgToOwnClient(RPY_ERR403_noSuchChannel(channel));
	}
	catch(notOnTheChannel &e)
	{
		(void)e;
		sendMsgToOwnClient(RPY_ERR442_youreNotOnThatChannel(channel));
	}
}

void 	User::setInviteOnly(const std::string& channel){
		std::cout << "set Channel " << channel <<" to invite only, if enough rights."<< std::endl;
		}

void 	User::remoInviteOnly(const std::string& channel){
		std::cout << "remove Channel " << channel <<" from invite only, if enough rights."<< std::endl;
		}

void 	User::setTopicRestrictions(const std::string& channel){
		std::cout << "set to Channel " << channel <<" topic restrictions, if enough rights."<< std::endl;
		}

void 	User::removeTopicRestrictions(const std::string& channel){
		std::cout << "remove from Channel " << channel <<" topic restrictions, if enough rights."<< std::endl;
		}

void 	User::setChannelKey(const std::string& channel, const std::string& key){
		std::cout << "set to Channel " << channel <<" password " << key << ", if enough rights."<< std::endl;
		}

void 	User::removeChannelKey(const std::string& channel){
		std::cout << "remove from Channel " << channel <<" password, if enough rights."<< std::endl;
		}

void 	User::giveChanopPrivileges(const std::string& channel, const std::string& username){
		std::cout << "Give user " << username << " chanop rights on Channel " << channel <<", if enough rights."<< std::endl;
		}

void 	User::removeChanopPrivileges(const std::string& channel, const std::string& username){
		std::cout << "Remove user " << username << " chanop rights on Channel " << channel <<", if enough rights."<< std::endl;
		}

void 	User::setUserLimit(const std::string& channel, int limit){
		std::cout << "Set userlimit on Channel " << channel <<" to " << limit << ", if enough rights."<< std::endl;
		}

void 	User::removeUserLimit(const std::string& channel){
		std::cout << "Remove userlimit on Channel " << channel <<", if enough rights."<< std::endl;
		}


void	User::mode(std::vector<std::string>& args)
{
 	modeParser parser(args);
	std::vector<std::pair<std::string, std::string> > flagArgsPairs = parser.getflagArgsPairs();
	std::vector<std::pair<std::string, std::string> > executedArgs;
	std::string channel = parser.getChannel();
	Channel *chptr = _server->getChannel(channel);

	for (size_t i = 0; i < flagArgsPairs.size(); ++i)
		std::cout << "\n\nFirst: " << flagArgsPairs[i].first << "\nSecond: " << flagArgsPairs[i].second << "\n\n";



	if (!chptr)
	{
		sendMsgToOwnClient(RPY_ERR403_noSuchChannel(channel));
		return;
	}
	User *usr = _server->getUser(_nickName);
	if (!usr)
		return;
	if (flagArgsPairs.size() == 0)
		printMode(channel, chptr);
	if (!chptr->isUserInList(chptr->getListPtrOperators(), usr))
	{
		sendMsgToOwnClient(RPY_ERR482_notChannelOp(channel));
		return;
	}

	for (size_t i = 0; i < flagArgsPairs.size(); ++i) {
		std::string flag = flagArgsPairs[i].first;
		std::string arguments = flagArgsPairs[i].second;

		//Test Output:
		std::cout << "\n\nFirst: " << flagArgsPairs[i].first << "\nSecond: " << flagArgsPairs[i].second << "\n\n";

		switch (flag[1]) {
			case 'i': //Set/remove Invite-only channel
				if (flag[0] == '+') {
					if (chptr->isModeSet(CHN_MODE_Invite, CHN_OPT_CTRL_NotExclusive) == false)
					{
						chptr->setMode(CHN_MODE_Invite);
						executedArgs.push_back(flagArgsPairs[i]);
					}
					setInviteOnly(channel);
				} else if (flag[0] == '-') {
					if (chptr->isModeSet(CHN_MODE_Invite, CHN_OPT_CTRL_NotExclusive) == true)
					{
						chptr->setMode(CHN_MODE_Invite);
						executedArgs.push_back(flagArgsPairs[i]);
					}
					remoInviteOnly(channel);
				}
				break;

			case 't': //Set/remove the restrictions of the TOPIC command to channel operators
				if (flag[0] == '+') {
					if (chptr->isModeSet(CHN_MODE_AdminSetTopic, CHN_OPT_CTRL_NotExclusive) == false)
					{
						chptr->setMode(CHN_MODE_AdminSetTopic);
						executedArgs.push_back(flagArgsPairs[i]);
					}
					setTopicRestrictions(channel);
				} else if (flag[0] == '-') {
					if (chptr->isModeSet(CHN_MODE_AdminSetTopic, CHN_OPT_CTRL_NotExclusive) == true)
					{
						chptr->setMode(CHN_MODE_AdminSetTopic);
						executedArgs.push_back(flagArgsPairs[i]);
					}
					removeTopicRestrictions(channel);
				}
				break;
			
			case 'k': //Set/remove the channel key (password)
				if (flag[0] == '+') {
					if (chptr->isModeSet(CHN_MODE_Protected, CHN_OPT_CTRL_NotExclusive) == true)
					{
						sendMsgToOwnClient(RPY_ERR467_keyAlreadySet(channel));
						break;
					}
					else
					{
						chptr->setMode(CHN_MODE_Protected);
						chptr->setPassword(arguments);
						executedArgs.push_back(flagArgsPairs[i]);
					}
					setChannelKey(channel, arguments);
				} else if (flag[0] == '-') {
					if (chptr->isModeSet(CHN_MODE_Protected, CHN_OPT_CTRL_NotExclusive) == true)
					{
						if (chptr->remPassword(arguments) == false)
						{
							sendMsgToOwnClient(RPY_ERR467_keyAlreadySet(channel));
							break;
						}
						executedArgs.push_back(flagArgsPairs[i]);
					}
					removeChannelKey(channel);
				}
				break;
			
			case 'o': //Give/take channel operator privilege
				{
					User *userptr = _server->getUser(arguments);
					if (!userptr)
					{
						sendMsgToOwnClient(RPY_ERR401_noSuchNickChannel(arguments));
						break;
					}
					if (flag[0] == '+') {
						if (chptr->isUserInList(chptr->getListPtrOperators(), userptr))
							break;
						if (!chptr->isUserInList(chptr->getListPtrOrdinaryUsers(), userptr))
						{
							sendMsgToOwnClient(RPY_ERR441_kickNotOnChannel(arguments, channel));
							break;
						}
						chptr->promoteUser(arguments);
						executedArgs.push_back(flagArgsPairs[i]);
						giveChanopPrivileges(channel, arguments);
					} else if (flag[0] == '-') {
						if (chptr->isUserInList(chptr->getListPtrOrdinaryUsers(), userptr))
							break;
						if (!chptr->isUserInList(chptr->getListPtrOperators(), userptr))
						{
							sendMsgToOwnClient(RPY_ERR441_kickNotOnChannel(arguments, channel));
							break;
						}
						chptr->demoteUser(arguments);
						executedArgs.push_back(flagArgsPairs[i]);
						removeChanopPrivileges(channel, arguments);
					}
				}
				break;

			case 'l': //Set/remove the user limit to channel
				if (flag[0] == '+') {
					if (arguments == "")
					{
						sendMsgToOwnClient(RPY_ERR461_notEnoughParameters(flag));
						break;
					}
					if (chptr->setChannelCapacity(std::atoi(arguments.c_str())) != 0)
					{
						break;
					}
					if (chptr->isModeSet(CHN_MODE_CustomUserLimit, CHN_OPT_CTRL_NotExclusive) == false)
						chptr->setMode(CHN_MODE_CustomUserLimit);
					executedArgs.push_back(flagArgsPairs[i]);
					setUserLimit(channel, std::atoi(arguments.c_str()));
				} else if (flag[0] == '-') {
					if (chptr->isModeSet(CHN_MODE_CustomUserLimit, CHN_OPT_CTRL_NotExclusive) == true)
						chptr->setMode(CHN_MODE_CustomUserLimit);
					executedArgs.push_back(flagArgsPairs[i]);
					removeUserLimit(channel);
				}
				break;	
			
			default:

				std::cout << "Unknown mode: " << flag << std::endl;
		}
	}
	if (!executedArgs.empty()) //Create the broadcast message
	{
		std::string createdFlags;
		std::string createdArgs;
		int			first = 1;

		for (size_t i = 0; i < executedArgs.size(); ++i)
		{
			std::string f = flagArgsPairs[i].first;
			std::string a = flagArgsPairs[i].second;
			if (first == 1)
			{
				createdFlags = flagArgsPairs[i].first;
				createdArgs = flagArgsPairs[i].second;
				first = 0;
			}
			else if (flagArgsPairs[i].first[0] == flagArgsPairs[i - 1].first[0])
			{
				createdFlags += flagArgsPairs[i].first[1];
				createdArgs += " " + flagArgsPairs[i].second;
			}
			else
			{
				createdFlags += flagArgsPairs[i].first;
				createdArgs += " " + flagArgsPairs[i].second;
			}
		}
		std::string message;
		message = ":" + _nickName + "!" + _userName + "@" + _ip + " MODE " + channel  + " " + createdFlags + " " + createdArgs;
		chptr->broadcastMsg(message, std::make_pair(false, (User *) NULL));
	}
}

void	User::printMode(std::string channel, Channel *ptr)
{
	std::string	combined;
	std::string flags = "+n";
	std::string args;
	if (ptr->isModeSet(CHN_MODE_Invite, CHN_OPT_CTRL_NotExclusive))
		flags += "i";
	if (ptr->isModeSet(CHN_MODE_CustomUserLimit, CHN_OPT_CTRL_NotExclusive))
	{
		flags += "l";
		std::ostringstream msgstream;
		msgstream << ptr->getChannelCapacity();
		args += msgstream.str() + " ";
	}
	if (ptr->isModeSet(CHN_MODE_Protected, CHN_OPT_CTRL_NotExclusive))
	{
		flags += "k";
		args += ptr->getPassword() + " ";
	}
	if (ptr->isModeSet(CHN_MODE_AdminSetTopic, CHN_OPT_CTRL_NotExclusive))
	{
		flags += "t";
		args += ptr->getTopic();
	}
	combined = flags + " " + args;
	sendMsgToOwnClient(RPY_324_printMode(channel, combined));
}

void		User::quitServer(std::vector<std::string>& args)
{
	if (_channelList.size() > 0)
	{
		(void) args;
		std::vector<Channel *>::iterator iter = _channelList.begin();
		std::vector<Channel *>::iterator iterTemp;

		while (iter != _channelList.end())
		{
			if ((*iter)->isUserInList((*iter)->getListPtrInvitedUsers(), this))
				(*iter)->updateUserList((*iter)->getListPtrInvitedUsers(), this, USR_REMOVE);
			if ((*iter)->isUserInList((*iter)->getListPtrOperators(), this))
			{
				(*iter)->broadcastMsg(RPY_leaveChannel((*iter)->getChannelName()), std::make_pair(false, (User *) NULL));
				(*iter)->updateUserList((*iter)->getListPtrOperators(), this, USR_REMOVE);
			}
			if ((*iter)->isUserInList((*iter)->getListPtrOrdinaryUsers(), this))
			{
				(*iter)->broadcastMsg(RPY_leaveChannel((*iter)->getChannelName()), std::make_pair(false, (User *) NULL));
				(*iter)->updateUserList((*iter)->getListPtrOrdinaryUsers(), this, USR_REMOVE);
			}
			if ((*iter)->isUserListEmpty((*iter)->getListPtrOperators()) && (*iter)->isUserListEmpty((*iter)->getListPtrOrdinaryUsers()))
			{
				iterTemp = iter;
				++iter;
				_server->remChannel((*iterTemp)->getChannelName());
			}
			else	
				++iter;
		}
	}
}


// //		*!* MESSAGES  *!*
// //		-----------------

/**
 * @brief 
 * function to send a notification to all users in a channel or one user.
 *
 * @param args std::vector < std::string >
 */
void	User::sendNotification(std::vector<std::string>& args)
{
	try
	{
		if (args[0].at(0) == '#')
		{
			std::cout << "User::sendNotification called with Channel =      " << args[0] << std::endl;

			Channel *chptr = _server->getChannel(args[0]);
			std::string concatenatedArgs = args[1];
			if (chptr != NULL){
            	for (std::vector<std::string>::const_iterator it = args.begin() + 2; it != args.end(); ++it)
            		{
                	if (it != args.begin() + 1)
                    concatenatedArgs += " ";
                	concatenatedArgs += *it;
            	}
				chptr->broadcastMsg(RPY_ChannelNotification(concatenatedArgs, chptr), std::make_pair(true, (User *) this));
			}
			else
				throw (noSuchChannel());
		}
		else
		{
			std::cout << std::endl << std::endl << "User::sendNotification called with nickname of target:   <" << args[0] << ">" << std::endl;

			User *target = _server->getUser(args[0]);
			if (target != NULL)
				sendMsgToTargetClient(RPY_PrivateNotification(args[1], target), target->getFd());
			else
				throw (noSuchNick());
		}
	}
	catch(noSuchChannel& e)
	{
		(void)e;
		sendMsgToOwnClient(RPY_ERR401_noSuchNickChannel(args[0]));
	}
	catch(noSuchNick& e)
	{
		(void)e;
		sendMsgToOwnClient(RPY_ERR401_noSuchNickChannel(args[0]));
	}
}

/**
 * @brief 
 * function to send a message to all users in the channel.
 *
 * @param args std::vector < std::string >
 */
int		User::sendChannelMsg(std::vector<std::string>& args)
{
	
	std::cout << "User::sendChannelMsg called with Channel =      " << args[0] << std::endl;
	try
	{
		Channel *chptr = _server->getChannel(args[0]);
		std::string concatenatedArgs = args[1];
		if (chptr != NULL){
            for (std::vector<std::string>::const_iterator it = args.begin() + 2; it != args.end(); ++it)
            {
                if (it != args.begin() + 1)
                concatenatedArgs += " ";
                concatenatedArgs += *it;
            }
			chptr->broadcastMsg(RPY_ChannelMsg(concatenatedArgs, chptr), std::make_pair(true, (User *) this));
		}
		else
			throw (noSuchChannel());
	}
	catch(noSuchChannel& e)
	{
		(void)e;
		sendMsgToOwnClient(RPY_ERR401_noSuchNickChannel(args[0]));
	}		
	return (0);
}

/**
 * @brief 
 * function to send a message to one user.
 *
 * @param args std::vector < std::string >
 */
int		User::sendPrivateMsg(std::vector<std::string>& args)
{
	std::cout << std::endl << std::endl << "User::sendPrivateMsg called. The nickname of target is:   <" << args[0] << ">" << std::endl;

	try
	{
		User *target = _server->getUser(args[0]);
		std::string concatenatedArgs = args[1];
		if (target != NULL)
		{
			for (std::vector<std::string>::const_iterator it = args.begin() + 2; it != args.end(); ++it)
            {
                if (it != args.begin() + 1)
                concatenatedArgs += " ";
                concatenatedArgs += *it;
            }
			sendMsgToTargetClient(RPY_PrivateMsg(concatenatedArgs, target), target->getFd());
		}
		else
			throw (noSuchNick());
	}
	catch(noSuchNick& e)
	{
		(void)e;
		sendMsgToOwnClient(RPY_ERR401_noSuchNickChannel(args[0]));
	}
	return (0);
}


// //		*!* OTHERS  *!*
// //		------------------------------------


std::string	User::argsToString(std::vector<std::string>::iterator iterBegin, std::vector<std::string>::iterator iterEnd)
{
	std::ostringstream msgstream;
	// if ((*iterBegin)[0] == ':')
	// {
	// 	msgstream << (*iterBegin).replace((*iterBegin).find(":"), 1, "");
	// 	++iterBegin;
	// 	if (iterBegin != iterEnd && iterBegin + 1 != iterEnd)
	// 		msgstream << " ";
	// }
	for (; iterBegin != iterEnd; ++iterBegin)
	{
		msgstream << *iterBegin;
		if (iterBegin + 1 != iterEnd)
			msgstream << " ";
	}
	return (msgstream.str());
}

void	User::removeFromChannelList(Channel *ptr)
{
	std::vector<Channel *>::iterator it = _channelList.begin();

	for (;it != _channelList.end(); ++it)
	{
		if (ptr == *it)
		{
			_channelList.erase(it);
			break;
		}
	}
}

bool	User::isUserInList(std::vector<User *>::iterator begin, std::vector<User *>::iterator end, User *user)
{
	std::vector<User *>::iterator it = begin;
	for (; it != end; ++it)
	{
		if (*it == user)
			return (true);
	}
	return (false);
}