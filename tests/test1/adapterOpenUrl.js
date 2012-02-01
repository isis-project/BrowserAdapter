 
	//Stress TestCases OpenUrl


	function loadUrl(){
		
   
                           
                        var myUrls = new Array();
                        myUrls[0] = "www.cnn.com";
                        myUrls[1] = "www.yahoo.com";
                        myUrls[2] = "www.msn.com";
						myUrls[3] = "www.google.com";
myUrls[4] = "www.ebay.com";
myUrls[5] = "www.espn.com";
myUrls[6] = "login.live.com";
myUrls[7] = "www.myspace.com";
myUrls[8] = "www.youtube.com";
myUrls[9] = "www.amazon.com";
myUrls[10] = "en.wikipedia.org";
myUrls[11] = "www.aol.com";
myUrls[12] = "walmart.com";
myUrls[13] = "maps.yahoo.com";
myUrls[14] = "www.imdb.com";
myUrls[15] = "www.target.com";
myUrls[16] = "address.mail.yahoo.com";
myUrls[17] = "www.weather.com";
myUrls[18] = "maps.google.com";
myUrls[19] = "www.mapquest.com";
myUrls[20] = "www.microsoft.com";
myUrls[21] = "gmail.google.com";
myUrls[22] = "www.msnbc.msn.com";
myUrls[23] = "www.aim.com";  
myUrls[24] = "www.amazon.com"; 
myUrls[25] = "mail.aol.com"; 
myUrls[26] = "www.aol.com"; 
myUrls[27] = "www.cnet.com"; 
myUrls[28] = "www.cnn.com"; 
myUrls[29] = "www.craigslist.org"; 
myUrls[30] = "www.ebay.com"; 
myUrls[31] = "www.espn.com"; 
myUrls[32] = "www.etrade.com"; 
myUrls[33] = "www.facebook.com";//login.php Login 
myUrls[34] = "www.flickr.com";//photos/  
myUrls[35] = "www.fandango.com"; 
myUrls[36] = "www.foxnews.com"; 
myUrls[37] = "www.google.com/ig"; //iGoogle 
myUrls[38] = "images.google.com"; 
myUrls[39] = "maps.google.com"; 
myUrls[40] = "www.hotmail.com"; 
myUrls[41] = "www.imdb.com"; 
myUrls[42] = "www.live.com"; 
myUrls[43] = "www.mapquest.com"; 
myUrls[44] = "www.msn.com"; 
myUrls[45] = "www.myspace.com"; 
myUrls[46] = "search.myspace.com/index.cfm?fuseaction=find";
myUrls[47] = "vids.myspace.com"; 
myUrls[48] = "www.nba.com"; 
myUrls[49] = "www.news.com"; 
myUrls[50] = "www.orbitz.com"; 
myUrls[51] = "www.weather.com"; 
myUrls[52] = "www.wikipedia.org"; //search result, use "palm" 
myUrls[53] = "mail.yahoo.com"; 
myUrls[54] = "www.yahoo.com"; 
myUrls[55] = "search.yahoo.com"; 
myUrls[56] = "www.youtube.com"; 
                       var rand= Math.floor(Math.random()*myUrls.length);
					   if( $('ran').checked)
		               var x= rand;
		                else
		                  x=i;
                        adapter.openURL(myUrls[x]);
						i++;		
					// set the delay	
                        if(i==myUrls.length){
							i=0;
							j++;
							}
                        console.log(myUrls[x] + " loop " + j+ " array " + x);
                        var t=setTimeout(loadUrl,5000);        
                     //number of loops
						if (j == $('loop').value) {
							clearTimeout(t);
							return;
						}    
						    
                        }
         
            var i=0;
			var j=0;
            loadUrl();

	
	