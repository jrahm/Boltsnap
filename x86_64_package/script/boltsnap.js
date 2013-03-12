var MAIN_CGI_SCRIPT="./cgi-bin/impulsecgi"

function filenode_on_click( filepath ) {
	request = new XMLHttpRequest();

	request.onreadystatechange= function() {
		if( request.readyState == 4 ) {
			if( request.status == 200 ) {
				console.log( request.responseText );
			} else {
				alert("There was an error on return");
			}
		}
	}

	filearg = encodeURIComponent( filepath );
	request.open("POST", MAIN_CGI_SCRIPT + "?file=" + filearg, true);
	request.send();
}
