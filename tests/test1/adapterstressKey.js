 //Regression TestCases
		
		
		function testRegression(){
			
			var rand1= Math.floor(Math.random()*320);
			var rand2= Math.floor(Math.random()*320);
			
			 var rand= Math.floor(Math.random()*3);    
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
adapter.setMagnification(scale * 2); 
var inter= adapter.inspectUrlAtPoint(rand1,rand2);
console.log("inspectUrlAtPoint = " + inter);
adapter.scrollTo(rand1,rand2);
break;
case 3:
adapter.clickAt(rand1,rand2); 
console.log ("clickAt " + rand1 + " "+ rand2);
		break;


default : 
adapter.openURL("www.google.com/");
console.log("end of loop " + j);
}           
                   if(i==3){
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
		
	
	