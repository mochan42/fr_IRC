/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fsemke <fsemke@student.42wolfsburg.de>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/05/04 21:22:03 by moninechan        #+#    #+#             */
/*   Updated: 2023/05/04 21:25:08 by fsemke           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include"../inc/Message.hpp"
#include"../inc/Utils.hpp"

bool isValidChannelName(const std::string& channelName) {
    size_t maxLength = 50; // According to RFC 2812, a channel name can be up to 50 characters long.

    if (channelName.length() < 2 || channelName.length() > maxLength) {
        return false;
    }
    for (size_t i = 1; i < channelName.length(); ++i) {
        if (channelName[i] == ' ' || channelName[i] == ',' || channelName[i] == '\x07') {
            return false;
        }
    }
    return true;
}
