<html>
	<head>
		<title>Boltsnap Media Player</title>
		<link href="style/index.css" rel="stylesheet" type="text/css">
	</head>
	<body>
		<script src="script/impulse.js"></script>
	
		<div class="center">
		<div class="wrapper">
		<div id="rootpanel">

		<div id="rootpanel-left">
		
		<div class="simplepanel">
			<p class="heading">Songs</p>
		</div>

		<div id="filenode_container">
			<?php
				$root="./media/music";
				
				function make_file_node( $file, $path ) {
					return "<div class=\"node filenode\" onclick=\"filenode_on_click( '{$path}' )\">
								<p>$file</p>
							</div>";
				}

				if($handle=opendir($root)) {
					echo "<ul class=\"media_list\">";
					while( false !== ($entry=readdir($handle)) ) {
						if( !( $entry=="." || $entry==".." ) ) {
							echo "<li>" . make_file_node( $entry, $root . "/" . $entry ) . "</li>";
						}
					}
					echo "</ul>";
				} else {
					echo "<p>Error Opening Directory!</p>";
				}
			?>
		</div>

		</div>
		</div>
		</div>
		</div>

	</body>
</html>
