# simpleHttpServer

Final Project for CSCD58

# Running the code

1. Set current working directory to Webserver and run 'gcc -pthread helpers.c webserver.c -o webserver.exe' to compile the code

2. run ./webserver.exe PORT_NUMBER
	- Replace PORT_NUMBER with the port you wish to run the server from

3. Open up a browser and type localhost:PORT_NUMBER to access the website

# Features

- Will keep connection open if keep-alive is specified, will not keep connection open if it is not specified
- Pipelining is implemented, will be able to take multiple requests at the same time, and process them in order.
- GET, POST, OPTIONS, DELETE, PATCH requests are implemented

## GET

- /: Gets the main page
- /main.css: Gets the styling for the main page
- /script.js: Gets some functionality for the main page
- /favicon.ico: Gets a tab icon for the main page
- /testpic.png: Gets a test image for the main page

## POST

- /item: Adds an item to db.txt

## OPTIONS

- /: Returns CORS headers for a cross-site request

## DELETE

- /item: Removes an item from db.txt

## PATCH

- /item: Replaces an item in db.txt