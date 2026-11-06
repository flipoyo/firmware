This was done in 2026 by Santiago Estrada (P26Estrada)

# Onboarding Robustel R3000 - LG4LA

This is the onboarding guide to the Gateway. I will do my best to make it understandable and funny so you don't get as bored as I did. Whenever an AI intervention occurs (which will happen often), I will mark it down so you know. Call it my work-ethic (I hate reading AI generated content).

## Step 1: Power

Before doing anything, just try plugging the gateway to the powergrid and hope no one broke it. We swear we left it up and running.

If you are rather short-minded like me, you will wonder how the hell we should connect it. For such end, you will need to use the provided power-supply. Since I know your directors (professors whose names I will not mention) will probably not remember the specifics, this image shows what we used:

![PowerSource](..\imgs\powerSource.jpeg)

Branching it is quite easy. You plug it in as the following image shows and that's it.

![Branching](..\imgs\PowerConnection.jpeg)

As soon as you do this, the led on the back that says `RUN` should start blinking every 2 seconds or so.

![powerOn](..\imgs\PowerOn.jpeg)

<sub> At this point, I'm sorry I did not crop the pictures... they take a lot of space but I must finish documenting soon. If you support documentation without AI for human entertaining, you may crop them for future **HUMAN** readers.

## Step 2: Connectivity

Great! You have power. It took humanity centuries to do what you did in 5 minutes. We are now missing connectivity. Take an ethernet cable (Mr. Flipo should have them. If not, ask the IT guys from Mines) and plug one end into `ETH0` and the other end to any wired-internet connection at Mines.

![Ethernet](..\imgs\ethernetConnection.jpeg)

The `NET` section of the Gateway should start blinking. When connecction is stablished, it blinks faster than the `RUN` LED.

### Oh no! It doesn't work!
- Notice there are some green and yellowish leds just below `ETH0` 