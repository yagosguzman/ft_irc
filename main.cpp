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
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        int port = 6667;  // Puerto predeterminado para IRC
        std::string password = "";

        // Si hay argumentos en la línea de comandos, los usamos
        if (argc == 3) {
            port = std::atoi(argv[1]);
            password = argv[2];
        } else if (argc == 2) {
            port = std::atoi(argv[1]);
        }

        // Crear instancia del servidor IRC
        serverIRC server(port, password);

        // Ejecutar el bucle principal del servidor
        server.run();
    } catch (const std::exception &e) {
        std::cerr << "Error fatal: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
