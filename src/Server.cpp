/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmeising <pmeising@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/14 21:10:05 by pmeising          #+#    #+#             */
/*   Updated: 2023/04/15 12:14:29 by pmeising         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
# include <exception>

//======== CONSTRUCTORS =========================================================================
Server::Server(unsigned int port, const std::string& password) :
    _port(port), _password(password), _errorFile("ErrorCodes.txt"), _operators() 

{
	try
	{
		this->createSocket();
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << "Error: server listening socket failed\n";
	}
}


//======== OVERLOAD OPERATORS ===================================================================


//======== DESTRUCTOR ===========================================================================
Server::~Server()
{
}

//======== GETTERS / SETTERS ====================================================================
unsigned int    Server::getPort(void) const
{
    return (this->_port);
}

void    Server::setPort(int inputPortNumber)
{
    this->_port = inputPortNumber;
}

const std::string	Server::getPassword(void) const
{
    return (this->_password);
}

int Server::getListeningSocket() const
{
	return (this->_listeningSocket);
}


//======== MEMBER FUNCTIONS =====================================================================

/* Creates a stream socket to receive incoming connections on */
/* AF_INET : for IPv4 protocol*/
/* We use TCP Protocol, hence SOCK_STREAM */
/* protocol = 0 beacuase there is only one protocol available for UNIX domain sockets */
void	Server::createSocket()
{
    this->_listeningSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (this->_listeningSocket == -1)
		throw ErrorInternal();
	else
		std::cout << "Listening Socket successfully opened : "  << this->getListeningSocket() << ".\n";
}

/* Allow listening socket file description to be reuseable */
void	Server::makeListeningSocketReusable()
{
	int	reuse, on = 1;

	reuse = setsockopt(this->_listeningSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
	if (reuse < 0)
		throw ErrorInternal();
	else
		std::cout << "Listening Socket successfully set to reusable.\n";
}

/* Set listening socket to be non blocking. All of the sockets for the incoming 
connections will also be non blocking since they will inherit that state
from the listening socket */
void	Server::setSocketToNonBlocking()
{
	int nonblock = 0;

	nonblock = fcntl(this->_listeningSocket, F_SETFL, O_NONBLOCK);
	if (nonblock == -1)
		throw ErrorInternal();
	else
		std::cout << "Listening Socket successfully set to non blocking.\n";
}

/* Bind the listening socket to the server port*/
void	Server::bindListeningSocketToServerPort(sockaddr_in addr)
{
	int	rbind;
	rbind = bind(this->getListeningSocket(), (sockaddr *)&addr, sizeof(sockaddr));
	if (rbind == -1)
		throw ErrorInternal();
	else
		std::cout << "Listening Socket sucessfully bound to server port.\n";
}


/* Listen the request from client (queues the requests). 
Backlog = 32 : how many request can be in the active queue */
void	Server::listenToClients()
{
	int rlisten;
	rlisten = listen(this->getListeningSocket(), 32);
	if (rlisten < 0)
		throw ErrorInternal();
	else
		std::cout << "Listening Socket started listening to IRC clients.\n";
}

//======== FUNCTIONS ============================================================================
int	checkIsDigit(char *s)
{
    for (int i = 0; s[i]; i++)
    {
        if (std::isdigit(s[i]) == 0)
        {
            std::cout << "Error!\n";
            return (1);
        }
    }
    return (0); 
}

int	checkOutOfRange(char *s)
{
	long	c;

	c = strtol(s, NULL, 10);
	if (c < MIN_PORT_NUMBER || c > MAX_PORT_NUMBER || strlen(s) > 11)
		return (1);
	return (0);
}

int	checkPort(char *port)
{
	int	err = 0;

	err = checkIsDigit(port) + checkOutOfRange(port);
	return (err);
}
