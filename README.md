Dídac Romero & Carles Homs

----------------------------------Multiplayer Game-----------------------------------
The game is the same as presented in the practice, with the same spaceships and mechanics.


-------------------------------Instructions------------------------------------------
Controls:
Same as the default ones for the practice.

KEYBOARD:
A,D ---------> to steer a spaceship
Arrow Key: DOWN -----------> to go forward
Arrow Key: LEFT -----------> to shoot a laser

-------------------------------Implemented Features-----------------------------------
WORLD STATE REPLICATION: Dídac	(Achieved with 2 known bugs)

BUG_1 : 
There's a known bug where whenever you shoot at a laser and it hits the player, the laser is not destroyed. 
This behaviour is common but it doesn't happen all the time. This bug could be caused by the fact that when
the server destroys a gameobject from the network the clients can't destroy it in time since the reference
in the network to said gameobject is null. Lasers always destroy on reaching their max lifetime.

BUG_2 :
Very rarely, a laser might not destroy until a new gameobject is placed in the network array where it belong,
so you might see that there are additional gameobjects in the network even if you have 1 spaceship. As tested,
there will only be 1 laser at max per client that takes longer to be destroyed (as described previously).


Reliability on top of UDP: Carles	(Achieved, altough not super confident it works flawlessly)

Redundancy and Acknowledgement work correctly: both client and server send packets with sequence ids in
order to keep track of what packet is sent and expected to receive in each side, re-sending packets if necessary
until confirmation of succesful delivery is recieved. When packets are not Acknowledged by the client, either by
an error or by too much time passing since it was sent, the server resends the packet until it is.

Improving Latency Handling: Carles	(Achieved, although lacking lag compensation)

Both the Client Prediction and the Interpolation of other Client GameObjects works as expected and no bugs
have been located. Client Prediction locally processes inputs until the client resyncs with the server on
each new Replication packet received, Interpolation does something similar with GameObjects controlled by
other players by using lerp() interpolation between the past state and the current one, effectively running
slightly behind the real world state. We record the time passed between each replication message in a buffer
in order to calculate the average, and use it as a reference time value for the lerp() (what time is 100%).