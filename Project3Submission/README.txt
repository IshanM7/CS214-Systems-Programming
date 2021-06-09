Names: Ishan Mody(iam47), Fiaz Mushfeq(fm398)

To test our project we made a client program to send data to the server and ensure correctness.
On top of the client we connected from different laptops to our server in order to test the multithreaded portion of the server.
We tested scenarios where we send info at the same time, for example trying to call GET while the other person calls a DEL in the middle
i.e:
    GET
    4
            DEL
            4
            day            
    day

In a case like this the person who typed GET first gets their value and then the client that typed delete deletes the key from the dataset.
to store the data we created a linked list in alphabetical order(When we insert we insert in alphabetical order).
To make sure our server was working we print out the list after every DEL or SET to make sure that the server is working as expected.
Also we tried many different combinations of actions to make sure the values came out as expected.
Some cases we tested:
    - trying to use an action while another client is in the middle of an action
    - setting a key that already exists(we updated the key with the new value)
    - bad input:
        get,sEt,wrong,-1,0,etc.
        These all would return ERR\nBAD\n
    - wrong lengths for the payload
    - deleting from head of a list, end of the list etc. to ensure deletes were working fine

The client connection gets closed for bad input such as wrong code and wrong length. In this case we preserve the linked list incase they want to 
reconnect with the server and access the data. The linked list gets free'd when the server is closed with an interrupt(^C) and all clients are disconnected.