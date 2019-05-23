#ifndef THALESREMOTECONNECTION_H
#define THALESREMOTECONNECTION_H

#include <string>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <queue>
#include <thread>
#include <mutex>

class ThalesRemoteConnection
{
public:

    ThalesRemoteConnection();

    /** Connect to Term (The Thales Terminal)
     *
     * \param [in] address the hostname or ip-address of the host running Term
     * \returns true on success, false if failed
     *
     * \todo actually just hangs if the host is up but Term has not been started.
     */
    bool connectToTerm(std::string address, std::string connectionName);

    /** Close the connection to Term and cleanup.
     *
     * Stops the thread used for receiving telegrams assynchronously and shuts down
     * the network connection.
     */
    void disconnectFromTerm();

    /** Check if the connection to Term is open.
     *
     * \returns true if connected, false if not.
     */
    bool isConnectedToTerm() const;

    /** Send a telegram (data) to Term)
     *
     * \param [in] payload the actual data which is being sent to Term.
     * \param [in] message_type used internally by the DevCli dll. Depends on context. Most of the time 2.
     */
    void sendTelegram(std::string payload, char message_type);

    /** Block infinitely until the next Telegram is arriving.
     *
     * If some Telegram has already arrived it will just return the last one from the queue.
     *
     * \returns the last received telegram or an empty string if someting went wrong.
     */
    std::string waitForTelegram();

    /** Block maximal <timeout> milliseconds while waiting for an incoming telegram.
     *
     * If some Telegram has already arrived it will just return the last one from the queue.
     *
     * \returns the last received telegram or an empty string if the timeout was reached or something went wrong.
     */
    std::string waitForTelegram(const std::chrono::duration<int, std::milli> timeout);

    /** Immediately return the last received telegram.
     *
     * \returns the last received telegram or an empty string if no telegram was received or something went wrong.
     */
    std::string receiveTelegram();

    /** Convenience function: Send a telegram and wait for it's reply.
     *
     * \param [in] payload the actual data which is being sent to Term.
     * \param [in] message_type used internally by the DevCli dll. Depends on context. Most of the time 2.
     * \returns the last received telegram or an empty string if someting went wrong.
     *
     * \warning If the queue is not empty the last received telegram will be returned. Recommended to flush the queue first.
     * \sa clearIncomingTelegramQueue();
     */
    std::string sendAndWaitForReply(std::string payload, char message_type);

    /** Checks if there is some telegram in the queue.
     *
     * \returns true if there is some telegram in the queue and false if not.
     */
    bool telegramReceived();

    /** Clears the queue of incoming telegrams.
     *
     * All telegrams received to this point will be discarded.
     *
     * \warning This does not stop new telegrams from being received after calling this method!
     */
    void clearIncomingTelegramQueue();

protected:

    static const int term_port = 260;

    int socket_handle;

    std::mutex receivedTelegramsGuard;
    std::queue<std::string> receivedTelegrams;

    std::timed_mutex telegramsAvailableMutex;

    bool receiving_worker_is_running;
    std::thread *receivingWorker;

    /** The method running in a separate thread, pushing the
     * incomming packets into the queue.
     */
    void telegramListenerJob();

    /** Starts the thread handling the asyncronously incoming data. */
    void startTelegramListener();

    /** Stops the thread handling the incoming data gracefully. */
    void stopTelegramListener();

    /** Reads the raw telegram structure from the socket stream. */
    std::string readTelegramFromSocket();

    /** Helper function getting the current time in milliseconds. */
    std::chrono::milliseconds getCurrentTimeInMilliseconds() const;

};

#endif // THALESREMOTECONNECTION_H
