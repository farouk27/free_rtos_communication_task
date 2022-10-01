# free RTOS communication task
Apply knowledge learned in the embedded programming part of the course and get hands on experience on <br />
RTOS concepts such as:<br />
1- Tasks<br />
2- Timers<br />
3- Queues<br />
4- Semaphores<br />


The project is implemented using FreeRTOS on the target emulation board provided via Eclipse CDT<br />
Embedded.<br />
Three tasks communicate via a queue of fixed size as described below:<br />
There are two sender tasks. Each sender task sleeps for a RANDOM period of time Tsender and when it<br />
wakes up it sends a message to the queue containing the string “Time is XYZ” where XYZ is current time in<br />
system ticks. If the queue is full, the sending operation fails and a counter counting total number of blocked<br />
messages is incremented. Upon successful sending, a counter counting total number of transmitted messages<br />
is incremented. The sender task is then blocked for another random period again. The random period is<br />
drawn from a uniform distribution as specified below.<br />
The receiver task sleeps for another FIXED period of time Treceiver and then wakes up and checks for any<br />
received message in the queue. If there is a message in the queue, it reads it, increments total number of<br />
received messages and sleeps again. If there is no message it sleeps again immediately. Note that receiver<br />
reads one message at a time even if there are more than one message in the queue.<br />
Embedded Systems Project 2022 Page 2 of 2 Revision: 1.0<br />
The sleep/wake control of the three tasks is performed via three timers one for each task. The callback<br />
function for each timer is specified as follows:<br />
Sender Timer Callback Function: When called it releases a dedicated semaphore on which the sender task is<br />
waiting/blocked on. The sender task is then unblocked and can send to the queue<br />
