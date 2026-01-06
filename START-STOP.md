Codul de mai jos este mai mult sau mai puțin un pseudocod. Trebuie să-l adaptați la tipul dvs. de microcontroler și la structura codului dvs. Îl puteți schimba așa cum vă place, dar trebuie să verificați dacă codul funcționează corect, astfel încât să nu existe penalizări dacă robotul nu se comportă așa cum ar trebui.

Pseudocodul nu este proiectat eficient deoarece fiecare utilizator are propriile idei de scriere a software-ului. Ar trebui privit ca un început pentru utilizatorii fără experiență care tocmai au început să programeze roboți. Codul folosește bucle while care țin robotul blocat în bucle diferite care nu sunt flexibile.

1.Când porniți robotul, trebuie să verificați dacă pinul de pornire este într-o stare de nivel „0”.

robot_ready_state = false;

// initial check

initial _start_pin_state = digitalRead(start_pin); 
if (initial_start__pin_state == 1) {  

// error, the start pin should be 0 when you power your robot
// if start pin state was “1” at the initial check you have to ask the referee to //press the  stop button and then power off and on your robot

    while(true) {

    // for safety reasons the robot is locked in this loop
    // you can flash a separate LED to let the user know
    // the only way to exit this loop is to do a power cycle
    }

} else if (initial_start_pin_state == 0) {

// correct state
// the robot is ready to accept the start command

 robot_ready_state = True;
}

2.Dacă verificarea inițială a trecut, puteți trece la următoarea parte în care trebuie să monitorizați starea pinului de pornire pentru a prinde comanda de pornire declanșată de arbitru.

current_start_pin_state = 0; 

while (current_start_pin_state  ==  0) { 
// keep the robot locked in a loop until the start command  is received

  current_start_pin_state = digitalRead(start_pin); 

}
 // the program will continue only if the start command is received

3. Când este primită comanda de pornire, robotul poate începe să opereze pe dohyo. Când este primită comanda de oprire, robotul ar trebui să oprească toate activitățile (deplasare, întoarcere).

If (current_start_pin_state  == 1 and robot_ready_state == True) {

  // the start command was received and the robot was in a ready state
  // this is where the robot will start to compete against the opponent
  // the if condition will not execute unless the robot is in a ready state.

  while (current_start_pin_state  ==  1) {

    // keep the robot locked in a loop until the stop command is received

    current_start_pin_state = digitalRead(start_pin);

  }

  // the program will continue only if the stop command is received

  If (current_start_pin_state  == 0) {

    // the stop command was received
    // this is where the robot should stop all operations

    while (true) {

      // for safety reasons the robot is locked in this loop
      // you can flash a separate LED to let the user know
      // the only way to exit this loop is to do a power cycle

    }

  }

}