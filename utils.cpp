/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ysanchez <ysanchez@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/29 18:15:39 by ysanchez          #+#    #+#             */
/*   Updated: 2025/01/29 19:18:15 by ysanchez         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "utils.hpp"

void check_args(std::string arg, int argnum)
{
	if (argnum == 1)
	{
		if (arg.size() > 10)
			throw std::length_error("Error: <port> CAN'T be longer than 10 digits");
		for (size_t i = 0; i < arg.length(); i++)
		{
			if (!isdigit(arg.at(i)))
				throw std::logic_error("Error: <port> MUST be a numerical input");
		}
		long port = strtol(arg.c_str(), NULL, 10);
		if (port > INT_MAX)
			throw std::range_error("Error: <port> CAN'T be bigger than INT_MAX");

	}
	else if (argnum == 2)
	{
		if (arg.size() > 20)
			throw std::length_error("Error: <password> CAN'T be longer than 20 characters");
		for (size_t i = 0; i < arg.length(); i++)
		{
			if (!isprint(arg.at(i)))
				throw std::logic_error("Error: <password> MUST use ONLY printable ASCII characters");
		}
	}
	else
		throw std::logic_error("Error: check_args wrong input");
}