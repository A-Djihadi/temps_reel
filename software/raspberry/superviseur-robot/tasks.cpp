/*
 * Copyright (C) 2018 dimercur
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "tasks.h"
#include <stdexcept>

// Déclaration des priorités des taches
#define PRIORITY_TSERVER 30
#define PRIORITY_TOPENCOMROBOT 20
#define PRIORITY_TMOVE 20
#define PRIORITY_TSENDTOMON 32
#define PRIORITY_TRECEIVEFROMMON 25
#define PRIORITY_TSTARTROBOT 20
#define PRIORITY_TSTARTROBOTWD 20
#define PRIORITY_WDRELOAD 24
#define PRIORITY_TCAMERA 21
#define PRIORITY_TBATTERY 15
#define PRIORITY_TCLOSECOMROBOT 14
#define PRIORITY_TSTARTCAMERA 28
#define PRIORITY_TPERIODICIMAGE 26
#define TIME_DELAY 1000000//1ms 

/*
 * Some remarks:
 * 1- This program is mostly a template. It shows you how to create tasks, semaphore
 *   message queues, mutex ... and how to use them
 * 
 * 2- semDumber is, as name say, useless. Its goal is only to show you how to use semaphore
 * 
 * 3- Data flow is probably not optimal
 * 
 * 4- Take into account that ComRobot::Write will block your task when serial buffer is full,
 *   time for internal buffer to flush
 * 
 * 5- Same behavior existe for ComMonitor::Write !
 * 
 * 6- When you want to write something in terminal, use cout and terminate with endl and flush
 * 
 * 7- Good luck !
 */

/**
 * @brief Initialisation des structures de l'application (tâches, mutex, 
 * semaphore, etc.)
 */
void Tasks::Init() {
    int status;
    int err;

    /**************************************************************************************/
    /* 	Mutex creation                                                                    */
    /**************************************************************************************/
    if (err = rt_mutex_create(&mutex_monitor, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_robot, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_robotStarted, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_robotStartedWD, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_move, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_camera, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    /* */
    cout << "Mutexes created successfully" << endl << flush;

    /**************************************************************************************/
    /* 	Semaphors creation       							  */
    /**************************************************************************************/
    if (err = rt_sem_create(&sem_barrier, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_openComRobot, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_closeComRobot, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_serverOk, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_startRobotWD, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_startRobot, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_watchdog, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_startCamera, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_periodicImage, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Semaphores created successfully" << endl << flush;

    /**************************************************************************************/
    /* Tasks creation                                                                     */
    /**************************************************************************************/
    if (err = rt_task_create(&th_server, "th_server", 0, PRIORITY_TSERVER, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_sendToMon, "th_sendToMon", 0, PRIORITY_TSENDTOMON, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_receiveFromMon, "th_receiveFromMon", 0, PRIORITY_TRECEIVEFROMMON, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_openComRobot, "th_openComRobot", 0, PRIORITY_TOPENCOMROBOT, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_startRobot, "th_startRobot", 0, PRIORITY_TSTARTROBOT, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_startRobotWD, "th_startRobotWD", 0, PRIORITY_TSTARTROBOTWD, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_wdReload, "th_wdReload", 0, PRIORITY_WDRELOAD, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_move, "th_move", 0, PRIORITY_TMOVE, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_battery, "th_battery", 0, PRIORITY_TBATTERY, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_startCamera, "th_startCamera", 0, PRIORITY_TSTARTCAMERA, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_periodicImage, "th_periodicImage", 0, PRIORITY_TPERIODICIMAGE, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_closeComRobot, "th_closeComRobot", 0, PRIORITY_TCLOSECOMROBOT, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
     /* 
     if (err = rt_task_create(&th_closeComMon, "th_closeComMon", 0, PRIORITY_TCLOSECOMMON, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }*/
    
    cout << "Tasks created successfully" << endl << flush;

    /**************************************************************************************/
    /* Message queues creation                                                            */
    /**************************************************************************************/
    if ((err = rt_queue_create(&q_messageToMon, "q_messageToMon", sizeof (Message*)*50, Q_UNLIMITED, Q_FIFO)) < 0) {
        cerr << "Error msg queue create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Queues created successfully" << endl << flush;

}

/**
 * @brief Démarrage des tâches
 */
void Tasks::Run() {
    rt_task_set_priority(NULL, T_LOPRIO);
    int err;

    if (err = rt_task_start(&th_server, (void(*)(void*)) & Tasks::ServerTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_sendToMon, (void(*)(void*)) & Tasks::SendToMonTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_receiveFromMon, (void(*)(void*)) & Tasks::ReceiveFromMonTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_openComRobot, (void(*)(void*)) & Tasks::OpenComRobot, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_startRobot, (void(*)(void*)) & Tasks::StartRobotTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_startRobotWD, (void(*)(void*)) & Tasks::StartRobotTaskWD, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_move, (void(*)(void*)) & Tasks::MoveTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_battery, (void(*)(void*)) & Tasks::LevelBattery, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_wdReload, (void(*)(void*)) & Tasks::Watchdog, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }    
    if (err = rt_task_start(&th_closeComRobot, (void(*)(void*)) & Tasks::CloseComRobot, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    cout << "Tasks launched" << endl << flush;
}

/**
 * @brief Arrêt des tâches
 */
void Tasks::Stop() {
    monitor.Close();
    robot.Close();
}

/**
 */
void Tasks::Join() {
    cout << "Tasks synchronized" << endl << flush;
    rt_sem_broadcast(&sem_barrier);
    pause();
}

/**
 * @brief Thread handling server communication with the monitor.
 */
void Tasks::ServerTask(void *arg) {
    int status;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are started)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task server starts here                                                        */
    /**************************************************************************************/
    rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
    status = monitor.Open(SERVER_PORT);
    rt_mutex_release(&mutex_monitor);

    cout << "Open server on port " << (SERVER_PORT) << " (" << status << ")" << endl;

    if (status < 0) throw std::runtime_error {
        "Unable to start server on port " + std::to_string(SERVER_PORT)
    };
    monitor.AcceptClient(); // Wait the monitor client
    cout << "Rock'n'Roll baby, client accepted!" << endl << flush;
    rt_sem_broadcast(&sem_serverOk);
}

/**
 * @brief Thread sending data to monitor.
 */
void Tasks::SendToMonTask(void* arg) {
    Message *msg;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task sendToMon starts here                                                     */
    /**************************************************************************************/
    rt_sem_p(&sem_serverOk, TM_INFINITE);

    while (1) {
        //cout << "wait msg to send" << endl << flush;
        msg = ReadInQueue(&q_messageToMon);
        //cout << "Send msg to mon: " << msg->ToString() << endl << flush;
        rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
        monitor.Write(msg); // The message is deleted with the Write
        rt_mutex_release(&mutex_monitor);
    }
}

/**
 * @brief Thread receiving data from monitor.
 */
void Tasks::ReceiveFromMonTask(void *arg) {
    Message *msgRcv;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task receiveFromMon starts here                                                */
    /**************************************************************************************/
    rt_sem_p(&sem_serverOk, TM_INFINITE);
    cout << "Received message from monitor activated" << endl << flush;

    while (1) {
        msgRcv = monitor.Read();
        cout << "Rcv <= " << msgRcv->ToString() << endl << flush;

        if (msgRcv->CompareID(MESSAGE_MONITOR_LOST)) {
            robot.Close();
            delete(msgRcv);
            exit(-1);
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_COM_OPEN)) {
            rt_sem_v(&sem_openComRobot);
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_COM_CLOSE)) {   
            rt_sem_v(&sem_closeComRobot);
        } else if (msgRcv->CompareID(MESSAGE_CAM_OPEN)) {
            rt_sem_v(&sem_startCamera);
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_START_WITHOUT_WD)) {
            rt_sem_v(&sem_startRobot);
        
        }else if (msgRcv->CompareID(MESSAGE_ROBOT_START_WITH_WD)) {
            rt_sem_v(&sem_startRobotWD); 
            
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_GO_FORWARD) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_BACKWARD) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_LEFT) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_RIGHT) ||
                msgRcv->CompareID(MESSAGE_ROBOT_STOP)) {

            rt_mutex_acquire(&mutex_move, TM_INFINITE);
            move = msgRcv->GetID();
            rt_mutex_release(&mutex_move);
        }
        delete(msgRcv); // mus be deleted manually, no consumer
    }
}


/**
 * @brief Thread closing server communication with the monitor.
 */
/*
void Tasks::CloseComMon(void *arg) {
    int status;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are started)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task closing communication with the monitor starts here                                                        */
    /**************************************************************************************/
    /*rt_sem_p(&sem_closeComMon, TM_INFINITE);

    rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
    monitor.Close();
    rt_mutex_release(&mutex_monitor);

    //rt_sem_broadcast(&sem_disconnectCam);
    rt_mutex_acquire(&mutex_robot, TM_INFINITE);
    move = MESSAGE_ROBOT_STOP;
    rt_mutex_release(&mutex_robot);
    
}*/

/**
 * @brief Thread opening communication with the robot.
 */
void Tasks::OpenComRobot(void *arg) {
    int status;
    int err; 
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task openComRobot starts here                                                  */
    /**************************************************************************************/
    while (1) {
        rt_sem_p(&sem_openComRobot, TM_INFINITE);
        cout << "Open serial com (";
        rt_mutex_acquire(&mutex_robot, TM_INFINITE);
        status = robot.Open();
        rt_mutex_release(&mutex_robot);
        cout << status;
        cout << ")" << endl << flush;

        Message * msgSend;
 
        if (status < 0) {
            msgSend = new Message(MESSAGE_ANSWER_NACK);
            cout << msgSend->ToString() << endl <<flush; 
        } else {
            msgSend = new Message(MESSAGE_ANSWER_ACK);
            cout << msgSend->ToString() << endl <<flush; 
        }
        WriteInQueue(&q_messageToMon, msgSend); // msgSend will be deleted by sendToMon
    }
}

/**
 * @brief Thread closing communication with the robot.
 */
void Tasks::CloseComRobot(void *arg) {
    int status;
    int err;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task closeComRobot starts here                                                  */
    /**************************************************************************************/
    while (1) {
        rt_sem_p(&sem_closeComRobot, TM_INFINITE);
        cout << "Close serial com (";
        rt_mutex_acquire(&mutex_robot, TM_INFINITE);
        status = robot.Close();
        rt_mutex_release(&mutex_robot);
        cout << status;
        cout << ")" << endl << flush;

        Message * msgSend;
        if (status < 0) {
            msgSend = new Message(MESSAGE_ANSWER_NACK);
        } else {
            msgSend = new Message(MESSAGE_ANSWER_ACK);
        }
        WriteInQueue(&q_messageToMon, msgSend); // msgSend will be deleted by sendToMon
    }
}



/**
 * @brief Thread starting the communication with the robot.
 */
void Tasks::StartRobotTask(void *arg) {

    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task startRobot starts here                                                    */
    /**************************************************************************************/
    while (1) {
        Message * msgSend;
        int compt=0;
        int reinit=1;
        
        rt_sem_p(&sem_startRobot, TM_INFINITE);
        cout << "Start robot without watchdog (";
        rt_mutex_acquire(&mutex_robot, TM_INFINITE);
        // MEDIUM CONTROLE
        while (compt<3 && reinit!=0) {
            msgSend = robot.Write(robot.StartWithoutWD());
            if (msgSend->GetID()==MESSAGE_ANSWER_COM_ERROR||msgSend->GetID()==MESSAGE_ANSWER_ROBOT_TIMEOUT||msgSend->GetID()==MESSAGE_ANSWER_ROBOT_UNKNOWN_COMMAND){ 
                compt++;
            }
            else {
                cout << "Message received" << compt << endl << flush;
                compt=0;
                reinit=0;
            }
        }
        cout << "Valeur du compteur " << compt << endl << flush;
        if (compt==3) {
            cout << "Connection Lost ";
            robot.Close();
        }
        //END MEDIUM CONTROLE
        
        rt_mutex_release(&mutex_robot);
        cout << msgSend->GetID();
        cout << ")" << endl;

        cout << "Movement answer: " << msgSend->ToString() << endl << flush;
        WriteInQueue(&q_messageToMon, msgSend);  // msgSend will be deleted by sendToMon

        if (msgSend->GetID() == MESSAGE_ANSWER_ACK) {
            rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
            robotStarted = 1;
            rt_mutex_release(&mutex_robotStarted);
        }
    }
}

/**
 * @brief Thread starting the communication with the robot.
 */
void Tasks::StartRobotTaskWD(void *arg) {
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task startRobot starts here                                                    */
    /**************************************************************************************/
    while(1) {
        Message * msgSend;
        int compt=0;
        int reinit=1;
        
        rt_sem_p(&sem_startRobotWD, TM_INFINITE);
        cout << "Start robot with WATCHDOG (";
        rt_mutex_acquire(&mutex_robot, TM_INFINITE);
        // MEDIUM CONTROLE
        while (compt<3 && reinit!=0) {
            msgSend = robot.Write(robot.StartWithWD());
            if (msgSend->GetID()==MESSAGE_ANSWER_COM_ERROR||msgSend->GetID()==MESSAGE_ANSWER_ROBOT_TIMEOUT||msgSend->GetID()==MESSAGE_ANSWER_ROBOT_UNKNOWN_COMMAND){ 
                //cout<< msgSend->ToString() << endl<<flush;
                cout << "LOST IN StartwithWD" <<endl <<flush;
                compt++;
            }
            else {
                //cout << "Message received" << compt << endl << flush;
                cout << "DONE"<<endl<<flush;
                compt=0;
                reinit=0;
            }
        }
        //cout << "Valeur du compteur " << compt << endl << flush;
        if (compt==3) {
            cout << "Connection Lost in StartwithWD" << endl;
            rt_sem_v(&sem_closeComRobot);
        }
        //END MEDIUM CONTROLE
        
        rt_mutex_release(&mutex_robot);
        
        cout << msgSend->GetID();
        cout << ")" << endl;
        
        cout << "Movement answer: " << msgSend->ToString() << endl << flush;
        WriteInQueue(&q_messageToMon, msgSend);  // msgSend will be deleted by sendToM
        
        if (msgSend->GetID() == MESSAGE_ANSWER_ACK) {
            rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
            robotStarted = 1;
            rt_mutex_release(&mutex_robotStarted);
            rt_sem_v(&sem_watchdog);
        }else{
            rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
            robotStarted = 0;
            rt_mutex_release(&mutex_robotStarted);
            }
    }
    
    
}
/**
 * @brief Thread starting watchdog for startRobotWithWD.
 */

void Tasks::Watchdog(void *arg) {

    int rs;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
  //Synchronization barrier (waiting that all tasks are starting and Robot is started)
    rt_sem_p(&sem_barrier, TM_INFINITE);
   /**************************************************************************************/
   /* The task starts here                                                               */
   /**************************************************************************************/
    
    rt_task_set_periodic(NULL, TM_NOW, 1000*TIME_DELAY);
    rt_sem_p(&sem_watchdog, TM_INFINITE);
    
    cout << "---------Reload Watchdog Task LAUNCHED------- " << endl << flush;
    while (1) {
        Message * msgSend;
        rt_task_wait_period(NULL);
        rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
        rs = robotStarted;
        rt_mutex_release(&mutex_robotStarted);
        
        if (rs == 1) {   
            cout << " <-----WATCHDOG RELOAD----> "<<endl<<flush;
            rt_mutex_acquire(&mutex_robot, TM_INFINITE);
            msgSend = robot.Write(robot.ReloadWD());
            rt_mutex_release(&mutex_robot);
            WriteInQueue(&q_messageToMon, msgSend);
            cout << "Reload :" << msgSend->ToString() << endl << flush ;
            
        }else{
            robot.Close();
        }
    }
}

/**
 * @brief Thread handling control of the robot.
 */
void Tasks::MoveTask(void *arg) {
    int rs;
    int cpMove;
    Message *msg;
    Message *msgSend;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task starts here                                                               */
    /**************************************************************************************/
    rt_task_set_periodic(NULL, TM_NOW, 100*TIME_DELAY);

    while (1) {
        int compt=0;
        int reinit=1;
        rt_task_wait_period(NULL);
        cout << "Periodic movement update"<< endl <<flush;
        rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
        rs = robotStarted;
        rt_mutex_release(&mutex_robotStarted);
        if (rs == 1) {
            rt_mutex_acquire(&mutex_move, TM_INFINITE);
            cpMove = move;
            rt_mutex_release(&mutex_move);
            
            cout << " Move: " << cpMove << endl<<flush;
            
            rt_mutex_acquire(&mutex_robot, TM_INFINITE);
            // MEDIUM CONTROLE
            while (compt<3 && reinit!=0) {
            
                msg = new Message((MessageID)cpMove);
                msgSend = robot.Write(msg);
                if (msgSend->GetID()==MESSAGE_ANSWER_COM_ERROR||msgSend->GetID()==MESSAGE_ANSWER_ROBOT_TIMEOUT||msgSend->GetID()==MESSAGE_ANSWER_ROBOT_UNKNOWN_COMMAND){ 
                    cout<< msgSend->ToString() << endl<<flush;
                    cout << "LOST IN MOVETASK" <<endl <<flush;
                    compt++;
                }
                else {
                    //cout <<endl<<flush;
                    cout << "Mesage received valeur du compteur " << compt << endl << flush;
                    //cout <<endl<<flush; 
                    compt=0;
                    reinit=0;
                    
                }
            }
            //cout << "Valeur du compteur " << compt << endl << flush;
            if (compt==3) {
                cout << "CONNECTION LOST " << endl;
                cout << endl << flush;
                robot.Close();
            }
            //END MEDIUM CONTROLE
            
            rt_mutex_release(&mutex_robot);
            WriteInQueue(&q_messageToMon, msgSend);
        };
        
        cout << endl << flush;
    }
}

/**
 * Write a message in a given queue
 * @param queue Queue identifier
 * @param msg Message to be stored
 */
void Tasks::WriteInQueue(RT_QUEUE *queue, Message *msg) {
    int err;
    if ((err = rt_queue_write(queue, (const void *) &msg, sizeof ((const void *) &msg), Q_NORMAL)) < 0) {
        cerr << "Write in queue failed: " << strerror(-err) << endl << flush;
        throw std::runtime_error{"Error in write in queue"};
    }
}

/**
 * Read a message from a given queue, block if empty
 * @param queue Queue identifier
 * @return Message read
 */
Message *Tasks::ReadInQueue(RT_QUEUE *queue) {
    int err;
    Message *msg;

    if ((err = rt_queue_read(queue, &msg, sizeof ((void*) &msg), TM_INFINITE)) < 0) {
        cout << "Read in queue failed: " << strerror(-err) << endl << flush;
        throw std::runtime_error{"Error in read in queue"};
    }/** else {
        cout << "@msg :" << msg << endl << flush;
    } /**/

    return msg;
}

void Tasks::LevelBattery(void *arg) {
    Message *msg;
    int rs;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task starts here                                                               */
    /**************************************************************************************/
    rt_task_set_periodic(NULL, TM_NOW, 500*TIME_DELAY);

    while (1) {
        int compt=0;
        int reinit=1;        
        rt_task_wait_period(NULL);
        
        cout << "Updated level of battery" <<endl<<flush;
        rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
        rs=robotStarted;
        rt_mutex_release(&mutex_robotStarted);
        
        if (rs == 1) {
        
            rt_mutex_acquire(&mutex_robot, TM_INFINITE);
            // MEDIUM CONTROLE
            while (compt<3 && reinit==1) {
                msg = robot.Write(robot.GetBattery());
                if (msg->GetID()==MESSAGE_ANSWER_COM_ERROR||msg->GetID()==MESSAGE_ANSWER_ROBOT_TIMEOUT||msg->GetID()==MESSAGE_ANSWER_ROBOT_UNKNOWN_COMMAND){ 
                    cout<< msg->GetID()<< endl<<flush;
                    cout << "LOST IN BATTERY" <<endl <<flush;
                    compt++;
                }
                else {
                    compt=0;
                    reinit=0;
                }
            }
            if (compt==3) {
                cout << "Dont get battery Connection Lost " <<endl <<flush;
                robot.Close();
            }
            //END MEDIUM CONTROLE
            rt_mutex_release(&mutex_robot);   
            //rt_mutex_acquire(&mutex_monitor, TM_INFINITE);  
            WriteInQueue(&q_messageToMon,msg);
            //rt_mutex_release(&mutex_monitor);
         }
        }
        cout << endl << flush;
    }

/**
 * @brief Thread starting Camera
 */
void Tasks::StartCameraTask(void *arg) {
    int status;
    Message *msgSend;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task starts here                                                               */
    /**************************************************************************************/
    rt_sem_p(&sem_startCamera, TM_INFINITE);

    rt_mutex_acquire(&mutex_camera, TM_INFINITE);
    status = camera.Open();
    rt_mutex_release(&mutex_camera);

    if(status){
        cout << "Rock'n'Roll baby, camera connected!" << endl << flush;
        rt_sem_broadcast(&sem_periodicImage);
        cout << "Periodic image started!" << endl << flush;
    } else {
        cout << "ERROR CONNECTING TO THE CAMERA" << endl << flush;
    }
}

/**
 * @brief Thread handling control of the battery level
 */
void Tasks::PeriodicImageTask(void *arg) {
    int co;
    MessageImg *toSend;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task starts here                                                               */
    /**************************************************************************************/
    rt_task_set_periodic(NULL, TM_NOW, 100000000);

    rt_sem_p(&sem_periodicImage, TM_INFINITE);

    while (1) {
        rt_task_wait_period(NULL);
        cout << "Periodic camera image" << endl << flush;

        rt_mutex_acquire(&mutex_camera, TM_INFINITE);
        co = camera.IsOpen();
        cout << "COUCOU" << endl << flush;
        if(co){
            cout << "Hi" << endl << flush;  
            Img img = camera.Grab();
            cout << "Hi" << endl << flush;  
            rt_mutex_release(&mutex_camera);
            toSend = new MessageImg(MESSAGE_CAM_IMAGE,&img);
            WriteInQueue(&q_messageToMon, toSend); 
            cout << "Image sent" << endl << flush;  
            sleep(1);

        } else {
            cout << "ERROR : Camera is not open yet" << endl << flush;    
            rt_mutex_release(&mutex_camera);
        }


    }
}



