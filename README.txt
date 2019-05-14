/**********************************************************************
 *
 * Flappy Bird 3D
 *
 * Authors: Brandon Sieu, Glenn Skelton
 * CPSC 599 Final Project, 2019
 *
 **********************************************************************/

** CCONTENTS **
application.cpp
Body.cpp
Body.h
Wing.cpp
Wing.h
rightWing.obj
leftWing.obj
birdBody.obj
application-VS2015.sln
application-VS2015.vcxproj
application-VS2015.vcxproj.filters
application-VS2015.vcxproj.user
background.jpg



** DESCRIPTION **
Flappy Bird 3D is an interactive game that uses haptic controllers to provide force feed-
back to the user. The game requires two Novint Falcon controllers in order to run. The 
game requires the user to flap the controllers (like a bird) to keep the bird avatar
aloft. The user flaps the controllers up and down to gain lift and depending on how
outstretched the controllers are (ie. spread apart from each other) will control how
effective the flapping is. If the user brings the controllers closer together (towards
the computer) the user will have less effective flapping and likewise if the controllers
are further apart their flapping will result in more lift. Also, if the user is not flapping
the placement of the controllers will affect the rate of descent. If the controllers are
closer together, the avatar will fall faster and if they are further apart, the avatar will
make a much more gradual descent. To win the game the user must fly through a series of
pipes. If a user hits the pipe, it is game over and the user must restart (arcade style).
If the user makes it past all of the pipes they win the game. There are 3 different level
difficulties; easy, medium, and hard. The easy level is recommended for beginners since the
pipe layout is much easier to navigate. The hard levels place the pipes closer together and
the gaps between the upper and lower pipes get much smaller. In each level, there are random
pockets of turbulence. These are randomly generated rumbles that are meant to provide extra 
challenge during gameplay and add extra haptic feedback to highten the experience of the 
game. 

** HOW TO COMPILE **
This application was developed in Microsoft Visual Studio and was developed in a Windows
environment. To compile, load into VS by selecting the solution file. The file folder
will need to be placed within a folder containing the chai3d library for the file 
paths.

** HOW TO USE **
In the main menu, the left and right Falcon buttons on either controller will toggle between 
menu options. To select an option, press the middle Falcon button. The same controls apply
to the level selection menu as well as the pause menu in the game. Once a level is selected
the user will experience a brief initialization period where the controllers will slowly ramp
up the joystick forces (to provide a smooth experience). Once in game play, the user may press
the middle Falcon button to pause. Once paused, the user can select to either resume, restart,
or quit the level. Additionally, the front Falcon button (closest to the device body) may be
pressed to automoatically restart the level.