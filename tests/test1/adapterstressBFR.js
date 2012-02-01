 //Regression TestCases
		
		
		function testRegression(){
			
				
			 var rand= Math.floor(Math.random()*9);    
			i++;     
	if( $('ran').checked)
		var x= rand;
		else
		x=i;			                
   switch (x){
case 1:
adapter.openURL("www.cnet.com/");
break;
case 2:
adapter.openURL("www.msn.com/");
break;
case 3:
console.log("canGoBack="+ " " + adapter.canGoBack());
break;
case 4:
adapter.goBack();
console.log("back");
break;
case 5:
console.log("canGoForward="+ " " + adapter.canGoForward()); 
break;
case 6:
adapter.goForward();
console.log("forward");
break;
case 7:		
        adapter.openURL( "http://www.cnn.com/" ); 
        adapter.stopLoad(); 
        console.log("stopLoad msn");
		break;
case 8:		
        adapter.reloadPage(); 
        console.log("reload");
		break;


default : 
adapter.openURL("www.google.com/");
console.log("end of loop " + j);
}           
                   if(i==9){
							i=0;
							j++;
							}
                        var t=setTimeout(testRegression,2000);
						        
                   //  number of loops
						if (j == $('loop').value) {
							clearTimeout(t);
							return;
						}    
			
		}
		    var i=0;
			var j=0;
		testRegression();
		
	
	