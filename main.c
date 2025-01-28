/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ysanchez <ysanchez@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/28 19:49:44 by ysanchez          #+#    #+#             */
/*   Updated: 2025/01/28 20:48:05 by ysanchez         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		std::cout << "Invalid number of arguments" << std::endl;
		return 1;
	}
	check_arg(argv[1]);
	check_arg(argv[2]);
	initserver();
	return 0;
}