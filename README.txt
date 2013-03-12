Boltsnap Readme
===============

	Executables
	-----------
	
		1.) boltsnapd

			| Main daemon responsible for reading and executing
			| commands.

		2.) playcgi
		
			| Cgi program used to send a play request to the daemon

		3.) controlcgi
			
			| Cgi program used to send a control request to the daemon.

	Compilation
	-----------
		
		| To compile the executables described above, run
		| `make`. To compile for arm, rum `make arm`

		| Make sure your system has the  libraries:
		|
		|	libasound-dev
		|	libswscale-dev
		|	libavformat-dev
		|	libavcodec-dev
		|	libao-dev
