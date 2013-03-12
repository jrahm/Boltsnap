#!/usr/bin/python

import cgitb
import cgi

import os
import sys

#cgitb.enable()
ACTION_COMMAND="action"
ARGUMENTS="args"
BIN_DIR="bin/"

# Parse the arguments of the 'arguments'
# field
def parseArgs( argumentStr ):
	ret = []; # List to return
	last = 0

	i = 0
	l = len(argumentStr)

	quotes = False
	while i < l:

		if argumentStr[i] == '\\':
			i += 1
		elif argumentStr[i] == "'":
			quotes = not quotes
		elif argumentStr[i] == ',' and not quotes:
			ret.append( argumentStr[last:i].replace("'",'');
			last = i + 1 # cut off ','
			
		i += 1
	return ret;

# Sanitize the string which was sent in
def sanitize( arg ):
	# return the base name. (No path attacks here!)
	return os.path.basename(arg);

# The main function for this code
def main( argv ):
	# the parameters sent in with the request
	params = cgi.FieldStorage()

	# the command to run
	if ACTION_COMMAND not in params:
		sys.stderr.write("No Action Command Specified! Bailing...\n")	
		exit(1)
	
	# The arguments to send to the executable
	args = []

	# Retrieve the action
	action = params[ACTION_COMMAND].value
	# Sanitize the action
	action = BIN_DIR + sanitize(action);
	
	if ARGUMENTS in params:
		# retrieve the arguments and parse them
		args = parseArgs( params[ARGUMENTS].value );
	
	# fork the program
	fork = os.fork();

	if fork == 0:
		# this is the child the child executes the new program
		print ("Executing " + action + " with arguments " + args );
		# insert the name of the executable
		args.insert( 0, action )
		os.execv( action, args );
		
		stderr.write("Error on execve.");
		print ("There was an error on execve")
	else:
		# the parent is going to wait for the child to finish
		sys.waitpid( fork );
	
	


if __name__ == "__main__":
	main( sys.argv );

