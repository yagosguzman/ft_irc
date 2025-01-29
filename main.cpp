/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ysanchez <ysanchez@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/28 19:49:44 by ysanchez          #+#    #+#             */
/*   Updated: 2025/01/29 19:17:36 by ysanchez         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "serverIRC.hpp"
#include "utils.hpp"
#include <stdlib.h>

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		std::cout << "Error: Invalid number of arguments => Use \'./ircserv <port> <password>\'" << std::endl;
		return 1;
	}
	for (int i = 1; i < argc; i++)
	{
		try
		{
			check_args(argv[i], i);
		}
		catch(const std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}	
	}
	// try
	// {
	// 	serverIRC servertest;
	// 	servertest.start();
	// }
	// catch(const std::exception& e)
	// {
	// 	std::cout << e.what() << std::endl;
	// }
	
	return 0;
}