#include "Game.h"

int main()
{
	srand(time(0));
	try 
	{
		Game game;
		game.run();
	}
	catch (std::exception& e)
	{
		std::cout << "EXCEPTION: " << e.what() << std::endl;
	}

	return 0;
}


//#include "Player.h"
//
//int main()
//{
//    srand(time(0));
//
//	sf::RenderWindow window(sf::VideoMode(800, 600), "Client");
//    window.setFramerateLimit(60);
//
//    sf::Clock clock;
//
//    Player player;
//
//    while (window.isOpen())
//    {
//        // check all the window's events that were triggered since the last iteration of the loop
//        sf::Event event;
//        while (window.pollEvent(event))
//        {
//            switch (event.type)
//            {
//            case sf::Event::Closed: 
//            {
//                window.close();
//            }
//            case sf::Event::KeyPressed:
//            {
//                
//            }
//            }
//        }
//
//        sf::Time elapsed = clock.restart();
//
//        window.clear(sf::Color::Black);
//
//        //draw
//        player.Update(window, elapsed);
//
//        window.display();
//    }
//
//	return 0;
//}