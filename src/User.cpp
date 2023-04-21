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
	this->_channelList = std::list<Channel*>();
	this->_isRegistered = false;
	this->_usernameSet = false;
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
	try
	{
		if (send(this->_userFd, msg.c_str(), msg.length(), 0) < 0)
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
	try
	{
		if (send(targetUserFd, msg.c_str(), msg.length(), 0) < 0)
			throw SendToTargetCLientException();
	}
	catch(const std::exception & e)
	{
		std::cerr << e.what() << '\n';
	}
	return (0);
}

/**
 * @brief 
 * function to call the received command.
 *
 * @param command std::string
 * @param args std::vector < std::string >
 */
void		User::executeCommand(std::string command, std::vector<std::string>& args)
{
	//Notice: Some Arguments like Pass can be written like: PASS, pass, Pass ... 

	std::cout << "User::executeCommand called." << std::endl;
	if (command == "CAP")
		;
	else if (command == "NICK")
		setNickName(args);
	else if (command == "USER")
		setUserName(args);
	else if (command == "REAL")
		setRealName(args);
	else if (command == "PASS")
		setServerPw(args);
	// else if (command == "JOIN")
	// 	joinChannel(args);
	// else if (command == "")
	// 	changeTopic(args);
	// else if (command == "")
	// 	inviteUser(args);
	// else if (command == "KICK")
	// 	kickUser(args);
	// else if (command == "PART")
	// 	leaveChannel(args);
	// else if (command == "")
	// 	modifyChannel(args);
	else if (command == "NOTICE")
		sendNotification(args);
	else if (command == "PRIVMSG")
	{
		if (args[0].at(0) == '#')
			sendMsg(args);
		else
			sendPrivateMsg(args);
	}
	// else if (command == "")
	// 	isOperator(args);
	else
	{
		this->sendMsgToOwnClient(RPY_ERR_commandNotfound(command));
		std::cout << _replyMessage << std::endl;
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

void		User::setServerPw(const std::vector<std::string>& args)
{
	_pw = args[0];
	if (this->_server->verifyPassword(_pw))
		_isRegistered = true;
}

void		User::setNickName(const std::vector<std::string>& args)
{
	_nickName = args[0];
	std::cout << "User::setNickname called. The _nickName is now:  " << _nickName << std::endl;
}

std::string	User::getNickName(void)
{
	return (this->_nickName);
}

void		User::setUserName(std::vector<std::string>& args)
{
	if (_usernameSet == true)
	{
		std::ostringstream msg;
		msg << ":" << _server->getServerName() << " 462 " << _nickName << " :Unauthorized command (already registered)";
		sendMsgToOwnClient(msg.str());
	}
	else
	{
		_userName = args[0];
		_realName = argsToString(args.begin() + 3, args.end());
	}
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


void		User::changeTopic(std::vector<std::string>& args)
{
	std::string channel = args[0];
	Channel *chnptr = _server->getChannel(channel);
	if (!chnptr)
	{
		//>> :master.ircgod.com 401 floNick #gdsafdsfa :No such nick/channel
		std::ostringstream msgadd;
		msgadd << ":" << _server->getServerName() << " 401 " << _nickName << " " << channel << " :No such nick/channel";
		sendMsgToOwnClient(msgadd.str());
	}
	if (args.size() == 1)
	{
		//WIP
	}
}


void 		User::inviteUser(std::vector<std::string>& args)
{
	std::string nick = args[0];
	std::string channel = args[1];

	User *user = _server->getUser(nick);
	Channel	*chptr = _server->getChannel(channel);
	if (!user || !channel)
	{
		//>> :master.ircgod.com 401 superman gfdsafas :No such nick/channel
		std::ostringstream msgadd;
		msgadd << ":" << _server->getServerName() << " 401 " << _nickName << " " << nick << " :No such nick/channel";
		sendMsgToOwnClient(msgadd.str());
	}
	else
	{
		if (chptr->isUserInChannel(nick) != NULL)
		{
			//>> :master.ircgod.com 443 superman superman#test :User is already on that channel
			std::ostringstream msgerr;
			msgerr << ":" << _server->getServerName() << " 443 " << _nickName << " " << nick << " " << channel << " :User is already on that channel";
			sendMsgToOwnClient(msgerr.str());
		}
		else
		{
			chptr->addUserToList(chptr->getListPtrInvitedUsers(), user);
			//>> :master.ircgod.com 341 superman floNick #test
			std::ostringstream msg;
			msg << ":" << _server->getServerName() << " 341 " << _nickName << " " << nick << " " << channel;
			sendMsgToOwnClient(msg.str());
			//!!!     write to other person:   !!!!
			//>> :<Nick>!<User@IP> INVITE <otherNick> <channel>
			std::ostringstream msg;
			msg << ":" << _nickName << "!" << _userName << "@" << _ip << " INVITE " << nick << channel;
			//Send to "otherNick" User
		}
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
	try
	{
		if (args[0][0] != '#')
			throw (badChannelMask());
		Channel *chptr = _server->getChannel(args[0]);
		if (chptr == NULL) //Create channel
		{
			chptr = _server->createChannel(args[0]);
			chptr->addUserToList(chptr->getListPtrOperators(), this);
		}
		else //join channel
		{
			// if (boolean ChannelKey == 1)
			// {
			// 	if (!args[1] || chptr->password != args[1])
			// 		throw (cannotJoinChannelPW());
			// }
			// if (boolean isInviteOnly == 1)
			// {
			// 	if (!chptr->isUserInList(chptr->getListPtrInvitedUsers(), this))
			// 		throw (cannotJoinChannelIn());
			// 	else
			// 		chptr->updateUserList(chptr->getListPtrInvitedUsers(), this, USR_REMOVE);
			// }
			// if (chptr->getChannelCapacity() <= chptr->getUserNum())
			// 	throw (channelCapacity());
			chptr->addUserToList(chptr->getListPtrOrdinaryUsers(), this);
		}
		std::ostringstream msgadd;
		msgadd << ":" << _nickName << "!" << _userName << "@" << _ip << " JOIN " << args[0];
		chptr->broadcastMsg(msgadd.str());
	}
	catch (badChannelMask &e)
	{
		(void)e;
		std::ostringstream msg;
		msg << ":" << _server->getServerName() << " 476 " << _nickName << " " << args[0] << " :Bad Channel Mask";
		sendMsgToOwnClient(msg.str());
	}
	catch (cannotJoinChannelPW &e)
	{
		(void)e;
		std::ostringstream msg;
		msg << ":" << _server->getServerName() << " 475 " << _nickName << " " << args[0] << " :Cannot join channel (+k)";
		sendMsgToOwnClient(msg.str());
	}
	catch (cannotJoinChannelIn &e)
	{
		(void)e;
		std::ostringstream msg;
		msg << ":" << _server->getServerName() << " 473 " << _nickName << " " << args[0] << " :Cannot join channel (+i)";
		sendMsgToOwnClient(msg.str());
	}
	catch (channelCapacity &e)
	{
		(void)e;
		std::ostringstream msg;
		msg << ":" << _server->getServerName() << " 471 " << _nickName << " " << args[0] << " :Cannot join channel (+l)";
		sendMsgToOwnClient(msg.str());
	}
}

/**
 * @brief 
 * Will kick a user from a channel and inform all other user on the channel about it.
 * @param args All arguments after the cmd
 */
// void		User::kickUser(std::vector<std::string>& args)
// {
// 	std::string channel = args[0];
// 	std::string nick = args[1];
// 	std::string reason = argsToString(args.begin() + 2, args.end());
// 	Channel *channelPtr;
// 	User	*tmpUser;
	
// 	try
// 	{
// 		channelPtr = _server->searchChannel(channel);
// 		if (!channelPtr->isUserInList(channelPtr->getListPtrOperators(), this))
// 			throw (notAnOperator());

// 		if (tmpUser = channelPtr->isUserInChannel(nick))
// 		{
// 			//write broadcastmessage: ":<Nick>!<Nick_USER@IP> KICK <channel> <kickedNick> <kickedNick>"
// 			std::ostringstream brmsg;
// 			brmsg << ":" << _nickName << "!" << _userName << "@" << _ip << " KICK " << channel << nick;
// 			//SEND MSG
			
// 			if (channelPtr->isUserInList(channelPtr->getListPtrOperators(), tmpUser))
// 				channelPtr->removeUserFromList(channelPtr->getListPtrOperators(), tmpUser);
// 			if (channelPtr->isUserInList(channelPtr->getListPtrOrdinaryUsers(), tmpUser))
// 				channelPtr->removeUserFromList(channelPtr->getListPtrOrdinaryUsers(), tmpUser);
// 		}
// 		else
// 			throw (notOnTheChannel());
// 	}
// 	catch (notAnOperator &e)
// 	{
// 		//write error: :<ServerName> 482 <Nick> <channel> :You're not channel operator
// 		std::ostringstream msg;
// 		msg << ":" <</* _server->getServerName() <<*/ " 482 " << _nickName << " " << channel << " :You're not channel operator";
// 		//SEND ERROR MESSAGE!!!
// 	}
// 	catch (notOnTheChannel &e)
// 	{
// 		//write error: ":<ServerName> 441 <Nick> <kickedNick> :Is not on channel <channel>"
// 		std::ostringstream msg;
// 		msg << ":" <</* _server->getServerName() <<*/ " 441 " << _nickName << " " << nick << " :Is not on channel " << channel;
// 		//SEND ERROR MSG!!!!
// 	}
// }

/**
 * @brief 
 * Function to leave a channel and inform all user in the channel about it.
 * @param args All arguments after the cmd
 */
// void		User::leaveChannel(std::vector<std::string>& args)
// {
// 	std::string	channel = args[0];
// 	Channel		*chPtr;

// 	//:<NICK>!<User@IP> PART <channel> :Leaving
// 	std::ostringstream msgstream;
// 	msgstream << ":" << _nickName << "!" << _userName << "@" << _ip << " PART " << channel << " :Leaving";
// 	try
// 	{
// 		//
// 		if (!(chPtr = _server->searchChannel(channel)))
// 			throw(noSuchChannel());
// 		if (chPtr->isUserInList(chPtr->getListPtrOrdinaryUsers(), this))
// 		{
// 			//broadcast msgstream
// 			chPtr->removeUserFromList(chPtr->getListPtrOrdinaryUsers(), this);
// 		}
// 		else if (chPtr->isUserInList(chPtr->getListPtrOperators(), this))
// 		{
// 			//broadcast msgstream
// 			chPtr->removeUserFromList(chPtr->getListPtrOperators(), this);
// 		}
// 		else
// 			throw(notOnTheChannel());
// 	}
// 	catch(noSuchChannel &e)
// 	{
// 		//:master.ircgod.com 403 nick1 #miau :No such channel
// 		std::ostringstream msg;
// 		msg << ":" <</* _server->getServerName() <<*/ " 403 " << _nickName << " " << channel << " :No such channel";
// 		//SEND ERROR MESSAGE!!!
// 	}
// 	catch(notOnTheChannel &e)
// 	{
// 		//:master.ircgod.com 442 nick1 #mychannel :You're not on that channel
// 		std::ostringstream msg;
// 		msg << ":" <</* _server->getServerName() <<*/ " 442 " << _nickName << " " << channel << " :You're not on that channel";
// 		//SEND ERROR MESSAGE!!!
// 	}
// }

// void		User::modifyChannel(std::string channelName, std::string nickName, char mode)
// {

// }

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
	if (args[0].at(0) == '#')
	{
		std::list<Channel *>::iterator iterChannel;
		// for (iterChannel = _channelList.begin(); iterChannel != _channelList.end(); iterChannel++)
		// {
			// if (args[0] == (*iterChannel)->getChannelName())     // with or without # at beginn ?
			// {
					std::ostringstream msgstream;
					std::vector<std::string>::iterator iterString = args.begin() + 1;
					msgstream << ":" << this->getNickName() << "!" << this->getUserName() << "@" << this->getIP() << " NOTICE " << args[0] << " ";
					for (; iterString != args.end(); ++iterString)
						msgstream << *iterString << " ";
					std::string msg = msgstream.str(); 
					std::cout << msg << std::endl;
				// (*iterChannel)->broadcastMsg(msg);     // send to all users in this channel but not to this user
				// std::cout << "User " << this->getUserName() << "with fd = " << this->getFd() << "sends a message to channel \'" << /*(*iterChannel)->getChannelName() << */ "\'." << std::endl;
			// 	break;
			// }
		// }
		// if (iterChannel == _channelList.end())
		// {
		// 	std::ostringstream msgstream;
		// 	msgstream << args[0] << " : No such Channel";
		// 	std::string msg = msgstream.str();
		// 	std::cout << msg << std::endl;
		// 	this->sendMsgToOwnClient(msg);
		// }
	}
	else
	{
		std::list<User *>::iterator iterUser;	
		// for (iterUser = _server.userList.begin(); iterUser != _server.userList.end(); iterUser++)
		// {
		// 	if (args[0] == (*iterUser)->getUserName())
		// 	{
				std::ostringstream msgstream;
				std::vector<std::string>::iterator iterString = args.begin() + 1;
				msgstream << ":" << this->getNickName() << "!" << this->getUserName() << "@" << this->getIP() << " NOTICE " << args[0] << " ";
				for (; iterString != args.end(); ++iterString)
				msgstream << *iterString << " ";
				std::string msg = msgstream.str();
				std::cout << msg << std::endl;
		// 		this->sendMsgToTargetClient(msg, (*iterUser)->getFd());

		// 		break;
				// std::cout << "User " << this->getUserName() << "with fd = " << this->getFd() << "sends a message to User \'" << /* args[2] << */ "\' with fd = " << /*args[3] << */ std::endl;
		// 	}
		// }
		// if (iterUser == _server.userList.end())
		// {
		// 	std::ostringstream msgstream;
		// 	msgstream << args[0] << " : No such user";
		// 	std::string msg = msgstream.str();
		// 	std::cout << msg << std::endl;
		// 	this->sendMsgToOwnClient(msg);
		// }
	}
}

/**
 * @brief 
 * function to send a message to all users in the channel.
 *
 * @param args std::vector < std::string >
 */
int		User::sendMsg(std::vector<std::string>& args)
{
	std::list<Channel *>::iterator iterChannel;
	// for (iterChannel = _channelList.begin(); iterChannel != _channelList.end(); iterChannel++)
	// {
		// if (args[0] == (*iterChannel)->getChannelName())     // with or without # at beginn ?
		// {
				std::ostringstream msgstream;
				std::vector<std::string>::iterator iterString = args.begin() + 1;
				msgstream << ":" << this->getNickName() << "!" << this->getUserName() << "@" << this->getIP() << " PRIVMSG " << args[0] << " ";
				for (; iterString != args.end(); ++iterString)
					msgstream << *iterString << " ";
				std::string msg = msgstream.str(); 
				std::cout << msg << std::endl;
			// (*iterChannel)->broadcastMsg(msg);     // send to all users in this channel but not to this user
			// std::cout << "User " << this->getUserName() << "with fd = " << this->getFd() << "sends a message to channel \'" << /*(*iterChannel)->getChannelName() << */ "\'." << std::endl;
		// 	break;
		// }
	// }
	// if (iterChannel == _channelList.end())
	// {
	// 	std::ostringstream msgstream;
	// 	msgstream << args[0] << " : No such Channel";
	// 	std::string msg = msgstream.str();
	// 	std::cout << msg << std::endl;
	// 	this->sendMsgToOwnClient(msg);
	// }
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
	std::list<User *>::iterator iterUser;
	
	// for (iterUser = _server.userList.begin(); iterUser != _server.userList.end(); iterUser++)
	// {
	// 	if (args[0] == (*iterUser)->getUserName())
	// 	{
			std::ostringstream msgstream;
			std::vector<std::string>::iterator iterString = args.begin() + 1;
			msgstream << ":" << this->getNickName() << "!" << this->getUserName() << "@" << this->getIP() << " PRIVMSG " << args[0] << " ";
			for (; iterString != args.end(); ++iterString)
			msgstream << *iterString << " ";
			std::string msg = msgstream.str();
			std::cout << msg << std::endl;
	// 		this->sendMsgToTargetClient(msg, (*iterUser)->getFd());

	// 		break;
			// std::cout << "User " << this->getUserName() << "with fd = " << this->getFd() << "sends a message to User \'" << /* args[2] << */ "\' with fd = " << /*args[3] << */ std::endl;
	// 	}
	// }
	// if (iterUser == _server.userList.end())
	// {
	// 	std::ostringstream msgstream;
	// 	msgstream << args[0] << " : No such user";
	// 	std::string msg = msgstream.str();
	// 	std::cout << msg << std::endl;
	// 	this->sendMsgToOwnClient(msg);
	// }
	return (0);
}


// //		*!* OTHERS  *!*
// //		------------------------------------


// std::string	User::sendPW()
// {

// }

std::string	User::argsToString(std::vector<std::string>::iterator iterBegin, std::vector<std::string>::iterator iterEnd)
{
	std::ostringstream msgstream;
	if ((*iterBegin)[0] == ':')
	{
		msgstream << (*iterBegin).replace((*iterBegin).find(":"), 1, "");
		++iterBegin;
		if (iterBegin != iterEnd && iterBegin + 1 != iterEnd)
			msgstream << " ";
	}
	for (; iterBegin != iterEnd; ++iterBegin)
	{
		msgstream << *iterBegin;
		if (iterBegin + 1 != iterEnd)
			msgstream << " ";
	}
	return (msgstream.str());
}
