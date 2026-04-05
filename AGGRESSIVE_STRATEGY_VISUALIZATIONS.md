# Sumo Robot AGGRESSIVE Strategy ASCII Visualizations

## Arena Setup
Each visualization shows a 50x50 character circular arena with the robot starting from the center, offset downward by 5.0 units.
The arena boundary is represented by '█' characters.

Legend:
- '█' = Arena boundary
- 'S' = Starting position
- 'E' = Ending position
- '●' = Primary path points
- '·' = Interpolated path (shows continuous movement)

## Strategy Details

## Strategy 0: AGGRESSIVE_SCAN
**Duration**: 1200ms: Oscillation -> Power alignment

```
                                                  
                                                  
                   █████████████                  
                ███████     ███████               
              ████               ████             
             ███                   ███            
           ███                       ███          
          ███                         ███         
         ██                             ██        
        ██                               ██       
       ██                                 ██      
      ██                                   ██     
      ██                                   ██     
     ██                                     ██    
    ██                                       ██   
    ██                                       ██   
   ██                                         ██  
   ██                                         ██  
   █                                           █  
  ██                                           ██ 
  ██                                           ██ 
  ██                                           ██ 
  ██                                           ██ 
  █                                             █ 
  █                                             █ 
  █                                             █ 
  █                                             █ 
  █                                             █ 
  ██                                           ██ 
  ██                                           ██ 
  ██                                           ██ 
  ██                  ···S                     ██ 
   █               ●··                         █  
   ██               ···                       ██  
   ██                  ··●                    ██  
    ██                   ·                   ██   
    ██                   ·                   ██   
     ██                  ·                  ██    
      ██                ·●                 ██     
      ██               ·●                  ██     
       ██              ··●                ██      
        ██           ····                ██       
         ██        ●·●                  ██        
          ███       ···               ███         
           ███         ··●           ███          
             ███         ·         ███            
              ████       ●       ████             
                ███████  ·  ███████               
                   █████████████                  
                    ●··●●E●                       
```

**AGGRESSIVE_SCAN**: Dramatic zigzag oscillation with hard turns and forward bursts.

## Strategy 1: HARD_PIVOT_CHARGE
**Duration**: 900ms: Hard pivot -> Maximum charge -> Alignment

```
                                                  
                                                  
                   █████████████                  
                ███████     ███████               
              ████               ████             
             ███                   ███            
           ███                       ███          
          ███                         ███         
         ██                             ██        
        ██                               ██       
       ██                                 ██      
      ██                                   ██     
      ██                                   ██     
     ██                                     ██    
    ██                                       ██   
    ██                                       ██   
   ██                                         ██  
   ██                                         ██  
   █                                           █  
  ██                                           ██ 
  ██                                           ██ 
  ██                                           ██ 
  ██                                           ██ 
  █                                             █ 
  █                                             █ 
  █                                             █ 
  █                                             █ 
  █                                             █ 
  ██                                           ██ 
  ██                                           ██ 
  ██           ●         S········●            ██ 
  ██           ·                  ·            ██ 
   █           ··                ·●            █  
   ██          ●·                ·            ██  
   ██          ··                ·            ██  
    ██         · ·              ·            ██   
    ██         · ·              ·            ██   
     ██        ●  ·●          ●·            ██    
      ██       ·   ··        ··            ██     
      ██       ·     ········              ██     
       ██      ●                          ██      
        ██     ·                         ██       
         ██    ·                        ██        
          ███  ·                      ███         
           ███ ●                     ███          
             ███                   ███            
              ████               ████             
                ███████     ███████               
                   █████████████                  
                                                  
```

**HARD_PIVOT_CHARGE**: 180° pivot rotation followed by maximum power charge.

## Strategy 2: POWER_SLIDE_ARC
**Duration**: 1100ms: Power slide arc -> Straightening -> Alignment

```
                                                  
                                                  
                   █████████████                  
                ███████     ███████               
              ████               ████             
             ███                   ███            
           ███                       ███          
          ███                         ███         
         ██                             ██        
        ██                               ██       
       ██                                 ██      
      ██                                   ██     
      ██                                   ██     
     ██                                     ██    
    ██                                       ██   
    ██                                       ██   
   ██                                         ██  
   ██                                         ██  
   █                                           █  
  ██                                           ██ 
  ██                                           ██ 
  ██                                           ██ 
  ██                                           ██ 
  █                                             █ 
  █                                             █ 
  █                                             █ 
  █                                             █ 
  █                                             █ 
  ██                                           ██ 
  ██                                           ██ 
  ██                                           ██ 
  ██                     S············         ██ 
   █                                ·●         █  
   ██                               ●         ██  
   ██                               ·         ██  
    ██                              ·        ██   
    ██                             ·●        ██   
     ██                            ·        ██    
      ██                          ·        ██     
      ██                         ·●        ██     
       ██                       ·●        ██      
        ██                    ··         ██       
         ██                ●··          ██        
          ███           ···           ███         
           ███         ·●            ███          
             ███       ·           ███            
              ████    ·          ████             
                ███████     ███████               
                  ●█████████████                  
                                                  
```

**POWER_SLIDE_ARC**: Wide curved arc with aggressive straightening maneuver.

## Strategy 3: DRAMATIC_BURST
**Duration**: 1100ms: Tense hold -> Turns -> Charge -> Alignment

```
                                                  
                                                  
                   █████████████                  
                ███████     ███████               
              ████               ████             
             ███                   ███            
           ███                       ███          
          ███                         ███         
         ██                             ██        
        ██                               ██       
       ██                                 ██      
      ██                                   ██     
      ██                                   ██     
     ██                                     ██    
    ██                                       ██   
    ██                                       ██   
   ██                                         ██  
   ██                                         ██  
   █                                           █  
  ██                                           ██ 
  ██                                           ██ 
  ██                                           ██ 
  ██                                           ██ 
  █                                             █ 
  █                                             █ 
  █                                             █ 
  █                                             █ 
  █                                             █ 
  ██                                           ██ 
  ██                                           ██ 
  ██                                           ██ 
  ██                   ●·S                     ██ 
   █                  ··                       █  
   ██               ··                        ██  
   ██             ··                          ██  
    ██          ···                          ██   
    ██          ··                           ██   
     ██          ●·                         ██    
      ██           ··                      ██     
      ██             ·●                    ██     
       ██             ···                 ██      
        ██              ·●               ██       
         ██              ·              ██        
          ███            ·            ███         
           ███           ·           ███          
             ███         ·         ███            
              ████       ·       ████             
                ███████  ●  ███████               
                   █████████████                  
                        ●E                        
```

**DRAMATIC_BURST**: Tense vibration hold leading to hard turns and maximum charge.

## Strategy 4: AGGRESSIVE_BAIT
**Duration**: 900ms: Reverse bait -> Forward slide -> Straightening -> Alignment

```
                                                  
                                                  
                   █████████████                  
                ███████     ███████               
              ████               ████             
             ███                   ███            
           ███                       ███          
          ███                         ███         
         ██                             ██        
        ██                               ██       
       ██                                 ██      
      ██                                   ██     
      ██                                   ██     
     ██                                     ██    
    ██                                       ██   
    ██                                       ██   
   ██                                         ██  
   ██                                         ██  
   █                                           █  
  ██                                           ██ 
  ██                                           ██ 
  ██                                           ██ 
  ██                                           ██ 
  █                                             █ 
  █             ●·                              █ 
  █              ●·                             █ 
  █               ··                            █ 
  █                ·●                           █ 
  ██                ··                         ██ 
  ██                 ··                        ██ 
  ██                  ●·                       ██ 
  ██                   ●·                      ██ 
   █                     S                     █  
   ██                    ··                   ██  
   ██                     ·●                  ██  
    ██                     ··                ██   
    ██                      ·●               ██   
     ██                     ·●              ██    
      ██                    ·              ██     
      ██                    ·●             ██     
       ██                   ·             ██      
        ██                  ·            ██       
         ██                 E           ██        
          ███                         ███         
           ███                       ███          
             ███                   ███            
              ████               ████             
                ███████     ███████               
                   █████████████                  
                                                  
```

**AGGRESSIVE_BAIT**: Reverse power slide bait followed by forward recovery.

## Strategy 5: POWER_RAMP_CHARGE
**Duration**: 1000ms: Exponential ramp -> Hard correction -> Power charge -> Alignment

```
                                                  
                                                  
                   █████████████                  
                ███████     ███████               
              ████               ████             
             ███                   ███            
           ███                       ███          
          ███                         ███         
         ██                             ██        
        ██                               ██       
       ██                                 ██      
      ██                                   ██     
      ██                                   ██     
     ██                                     ██    
    ██                                       ██   
    ██                                       ██   
   ██                                         ██  
   ██                                         ██  
   █                                           █  
  ██                                           ██ 
  ██                                           ██ 
  ██                                           ██ 
  ██                                           ██ 
  █                                             █ 
  █                                             █ 
  █                                             █ 
  █                                             █ 
  █                                             █ 
  ██                                           ██ 
  ██                                           ██ 
  ██                                           ██ 
  ██                     S                     ██ 
   █                     ●                     █  
   ██                    ●                    ██  
   ██                    ●                    ██  
    ██                   ●                   ██   
    ██                   ●                   ██   
     ██                  ·                  ██    
      ██                 ·                 ██     
      ██                 ·                 ██     
       ██                ·                ██      
        ██               ●               ██       
         ██              ·              ██        
          ███            ·            ███         
           ███           ·           ███          
             ███     ●····         ███            
              ████    ·          ████             
                ███████     ███████               
                   █████████████                  
                     ●●E                          
```

**POWER_RAMP_CHARGE**: Exponential acceleration ramp with weaving pattern.

## Strategy 6: WIDE_SWEEP_BURST
**Duration**: 800ms: Sweeps -> Weaving burst -> Straight charge

```
                                                  
                                                  
                   █████████████                  
                ███████     ███████               
              ████               ████             
             ███                   ███            
           ███                       ███          
          ███                         ███         
         ██                             ██        
        ██                               ██       
       ██                                 ██      
      ██                                   ██     
      ██                                   ██     
     ██                                     ██    
    ██                                       ██   
    ██                                       ██   
   ██                                         ██  
   ██                                         ██  
   █                                           █  
  ██                                           ██ 
  ██                                           ██ 
  ██                                           ██ 
  ██                                           ██ 
  █                                             █ 
  █                                             █ 
  █                                             █ 
  █                                             █ 
  █                                             █ 
  ██                                           ██ 
  ██                                           ██ 
  ██                                           ██ 
  ██                  ···S                     ██ 
   █               ●··                         █  
   ██           ···                           ██  
   ██        ●··                              ██  
    ██        ····                           ██   
    ██           ●·●·                        ██   
     ██             ···●                    ██    
      ██               ···                 ██     
      ██                 ·                 ██     
       ██                ·                ██      
        ██               ·               ██       
         ██              ·              ██        
          ███            ·            ███         
           ███           ·           ███          
             ███         ●         ███            
              ████       ·       ████             
                ███████  ·  ███████               
                   █████████████                  
                        E●                        
```

**WIDE_SWEEP_BURST**: Wide left/right sweeps transitioning to weaving burst.

## Strategy 7: EXPERIMENTAL_SLIDE
**Duration**: 1000ms: Power slide -> Recovery turn -> Maximum charge -> Alignment

```
                                                  
                                                  
                   █████████████                  
                ███████     ███████               
              ████               ████             
             ███                   ███            
           ███                       ███          
          ███                         ███         
         ██                             ██        
        ██                               ██       
       ██                                 ██      
      ██                                   ██     
      ██                                   ██     
     ██                                     ██    
    ██                                       ██   
    ██                                       ██   
   ██                                         ██  
   ██                                         ██  
   █                                           █  
  ██                                           ██ 
  ██                                           ██ 
  ██                                           ██ 
  ██                                           ██ 
  █                                             █ 
  █                                             █ 
  █                                             █ 
  █                                             █ 
  █                                             █ 
  ██                                           ██ 
  ██                                           ██ 
  ██                                           ██ 
  ██                     S·●···●····●·         ██ 
   █                                ·          █  
   ██                             ··          ██  
   ██                            ··           ██  
    ██                          ··           ██   
    ██                         ·             ██   
     ██                        ·            ██    
      ██                       ·           ██     
      ██                       ·           ██     
       ██                      ·          ██      
        ██                     ·         ██       
         ██                    ·        ██        
          ███                  ·      ███         
           ███                 ·     ███          
             ███               ·   ███            
              ████             · ████             
                ███████     ███████               
                   █████████████                  
                               E●                 
```

**EXPERIMENTAL_SLIDE**: Power slide with recovery turn and maximum forward charge.

---
*Generated by strategy_visualizer.py*