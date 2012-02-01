 //Regression TestCases
		
		
		function testRegression(){
			
			 var rand= Math.floor(Math.random()*24);    
			i++;     
	if( $('ran').checked)
		var x= rand;
		else
		x=i;			                
   switch (x){
case 1:
adapter.openURL("www.yahoo.com/");
break;
case 2:
adapter.openURL("www.cnn.com/");
break;
case 3:
console.log("canGoBack="+ " " + adapter.canGoBack());
break;
case 4:
adapter.goBack();
break;
case 5:
console.log("canGoForward="+ " " + adapter.canGoForward()); 
break;
case 6:
adapter.goForward();
break;
case 7:
adapter.setMagnification(scale * 1.25); 
        console.log("setMagnification * 1.25"); 
		break;
case 8:       
        adapter.setMagnification(scale / 1.25); 
        console.log("setMagnification / 1.5"); 
		break;
case 9:       
        adapter.pageScaleAndScroll( 2, 10, 10 );         
        console.log("pageScaleAndScroll * 2");
		break;
case 10:		
		 
        var str1 = adapter.smartZoom(50,50);		
        console.log("smartZoom"); 
		break;
case 11:		
        adapter.openURL( "http://www.msn.com/" ); 
        adapter.stopLoad(); 
        console.log("stopLoad msn");
		break;
case 12:		 
        adapter.setZoom(4); 
		console.log("SetZoom"); 
		break;
case 13:		
        adapter.reloadPage(); 
        console.log("reload");
		break;
case 14:		 
        adapter.setMinFontSize(10);
		break;
case 15:		         
        adapter.scrollTo(400,400);        
        console.log("scrollTo(400,400)");
		break;
case 16:		 
        adapter.scrollBy(110,110);
		break;
case 17:		 
        var inter= adapter.inspectUrlAtPoint(50,50);
        console.log("inspectUrlAtPoint 50,50 = " + inter); 
		break;
 case 18:       
        var inter1=adapter.isInteractiveAtPoint(50,50);
        console.log("isInteractiveAtPoint 50, 50 " + inter1); 
		break;
 case 19:  	
        var inter2 = adapter.interrogateClicks('Y'); 
        console.log ("interrogateClicks = " + inter2); 
		break;
case 20:  		
        adapter.setPageIdentifier("yahoo!"); 
		break;
case 21:  		
        adapter.clearCache(); 
		console.log ("clearCache");
		break;
case 22:  		
        adapter.clearCookies(); 
		console.log ("clearCookies");
		break;
case 23:  		
        adapter.clickAt(50,50); 
		console.log ("clickAt 50 , 50");
		break;
		
default : 
adapter.openURL("www.google.com/");
console.log("end of loop " + j);
}           
                   if(i==24){
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
		
	
	