#include <stdio.h>
#include <mpi.h>
#include <stdbool.h>

int main(int argc, char *argv[])
{
    int rank, size;
    MPI_Init(&argc, &argv);               // Initializes MPI
    MPI_Comm_size(MPI_COMM_WORLD, &size); // Figures out the number of processors I am asking for
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Figures out which rank we are

    bool isActive[4] = {true, true, true, true}; // if a process is active or passive
    bool isElected = false;                      // if a process has been elected leader

    MPI_Request ireq1, ireq2, ireq3, ireq4;
    MPI_Status istatus;

    // use sample rankIDs
    int rankID[4] = {50, 102, 75, 98}; // hard-coded IDs, max 4 processors
    int outMsg1, outMsg2, leftMsg, rightMsg;
    //int msg1, msg2, msg3, msg4, msg5, msg6, msg7, msg8;

    // multiple election rounds until one node elected as leader
    while (!isElected)
    {
        //msg1 = msg2 = msg3 = msg4 = msg5 = msg6 = msg7 = msg8 = rankID[rank];
        outMsg1 = outMsg2 = rankID[rank];
        printf("Process %d has ID = %d\n", rank, rankID[rank]);
        if (isActive[rank]) // if current process is active
        {
            // rank 0 must send to next rank and last node in ring
            if (rank == 0)
            {
                // send rankID msg to both neighbor nodes
                MPI_Isend(&outMsg1, 1, MPI_INT, 1, 1, MPI_COMM_WORLD, &ireq1);
                MPI_Isend(&outMsg2, 1, MPI_INT, size - 1, 1, MPI_COMM_WORLD, &ireq2);
                // receive rankID msgs from both neighbor nodes
                MPI_Irecv(&rightMsg, 1, MPI_INT, 1, 1, MPI_COMM_WORLD, &ireq3);
                MPI_Irecv(&leftMsg, 1, MPI_INT, size - 1, 1, MPI_COMM_WORLD, &ireq4);
                // wait for receive operations to complete to guarantee message delivery
                MPI_Wait(&ireq3, &istatus);
                MPI_Wait(&ireq4, &istatus);
            }
            // last node in ring must send to rank 0 and previous rank
            else if (rank == size - 1)
            {
                MPI_Isend(&outMsg1, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &ireq1);
                MPI_Isend(&outMsg2, 1, MPI_INT, rank - 1, 1, MPI_COMM_WORLD, &ireq2);
                MPI_Irecv(&rightMsg, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &ireq3);
                MPI_Irecv(&leftMsg, 1, MPI_INT, rank - 1, 1, MPI_COMM_WORLD, &ireq4);
                MPI_Wait(&ireq3, &istatus);
                MPI_Wait(&ireq4, &istatus);
            }
            // all other nodes must send to next rank and previous rank
            else
            {
                MPI_Isend(&outMsg1, 1, MPI_INT, rank + 1, 1, MPI_COMM_WORLD, &ireq1);
                MPI_Isend(&outMsg2, 1, MPI_INT, rank - 1, 1, MPI_COMM_WORLD, &ireq2);
                MPI_Irecv(&rightMsg, 1, MPI_INT, rank + 1, 1, MPI_COMM_WORLD, &ireq3);
                MPI_Irecv(&leftMsg, 1, MPI_INT, rank - 1, 1, MPI_COMM_WORLD, &ireq4);
                MPI_Wait(&ireq3, &istatus);
                MPI_Wait(&ireq4, &istatus);
            }

            // if current node is lesser than any of its neighbor nodes
            if (rightMsg > rankID[rank] || leftMsg > rankID[rank])
            {
                // node becomes passive
                isActive[rank] = false;
                printf("Process %d has become passive\n", rank);
            }
            // if receiving msgs are same ID as current node, then it is the last active node,
            // so it is elected as leader
            else if (rightMsg == rankID[rank] || leftMsg == rankID[rank])
            {
                isElected = true;
                printf("Process %d has been elected as a leader with ID %d\n", rank, rankID[rank]);
            }
        }
        else // process is passive, so forward incoming msg to next node
        {
            // passive node receives message from neighbor and sends same buffer to next node
            if (rank == 0)
            {
                // receive rankID msgs from neighbor nodes
                MPI_Irecv(&rightMsg, 1, MPI_INT, 1, 1, MPI_COMM_WORLD, &ireq3);
                MPI_Irecv(&leftMsg, 1, MPI_INT, size - 1, 1, MPI_COMM_WORLD, &ireq4);
                // forward the same msg buffer to next node
                MPI_Isend(&rightMsg, 1, MPI_INT, size - 1, 1, MPI_COMM_WORLD, &ireq1);
                MPI_Isend(&leftMsg, 1, MPI_INT, 1, 1, MPI_COMM_WORLD, &ireq2);
                // wait for receive operations to complete to guarantee message delivery
                MPI_Wait(&ireq3, &istatus);
                MPI_Wait(&ireq4, &istatus);
            }
            else if (rank == size - 1)
            {
                MPI_Irecv(&rightMsg, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &ireq3);
                MPI_Irecv(&leftMsg, 1, MPI_INT, rank - 1, 1, MPI_COMM_WORLD, &ireq4);
                MPI_Isend(&rightMsg, 1, MPI_INT, rank - 1, 1, MPI_COMM_WORLD, &ireq1);
                MPI_Isend(&leftMsg, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &ireq2);
                MPI_Wait(&ireq3, &istatus);
                MPI_Wait(&ireq4, &istatus);
            }
            else
            {
                MPI_Irecv(&rightMsg, 1, MPI_INT, rank + 1, 1, MPI_COMM_WORLD, &ireq3);
                MPI_Irecv(&leftMsg, 1, MPI_INT, rank - 1, 1, MPI_COMM_WORLD, &ireq4);
                MPI_Isend(&rightMsg, 1, MPI_INT, rank - 1, 1, MPI_COMM_WORLD, &ireq1);
                MPI_Isend(&leftMsg, 1, MPI_INT, rank + 1, 1, MPI_COMM_WORLD, &ireq2);
                MPI_Wait(&ireq3, &istatus);
                MPI_Wait(&ireq4, &istatus);
            }
        }
    }

    MPI_Abort(MPI_COMM_WORLD, MPI_SUCCESS); // terminate processes after leader is elected
    MPI_Finalize();                         // shutdown MPI
    return 0;
}