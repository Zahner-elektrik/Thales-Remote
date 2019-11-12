/******************************************************************
 *  ____       __                        __    __   __      _ __
 * /_  / ___ _/ /  ___  ___ ___________ / /__ / /__/ /_____(_) /__
 *  / /_/ _ `/ _ \/ _ \/ -_) __/___/ -_) / -_)  '_/ __/ __/ /  '_/
 * /___/\_,_/_//_/_//_/\__/_/      \__/_/\__/_/\_\\__/_/ /_/_/\_\
 *
 * Copyright 2019 ZAHNER-elektrik I. Zahner-Schiller GmbH & Co. KG
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
 * THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef THALESREMOTECONNECTION_H
#define THALESREMOTECONNECTION_H

#include <string>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <queue>
#include <thread>
#include <mutex>
#include <vector>

#ifdef _WIN32

#define MSG_MORE 0
#define SHUT_RD 0

#include <ws2tcpip.h>
#include <windows.h>

#else

#define SOCKET int
#define INVALID_SOCKET -1

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#endif

class ThalesRemoteConnection
{
public:

    ThalesRemoteConnection();
    ~ThalesRemoteConnection();

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
    void sendTelegram(std::vector<unsigned char> payload, unsigned char message_type);

    /** Block infinitely until the next Telegram is arriving.
     *
     * If some Telegram has already arrived it will just return the last one from the queue.
     *
     * \returns the last received telegram or an empty string if someting went wrong.
     */
    std::string waitForStringTelegram();
    std::vector<uint8_t> waitForTelegram();

    /** Block maximal <timeout> milliseconds while waiting for an incoming telegram.
     *
     * If some Telegram has already arrived it will just return the last one from the queue.
     *
     * \returns the last received telegram or an empty string if the timeout was reached or something went wrong.
     */
    std::string waitForStringTelegram(const std::chrono::duration<int, std::milli> timeout);
    std::vector<uint8_t> waitForTelegram(const std::chrono::duration<int, std::milli> timeout);

    /** Immediately return the last received telegram.
     *
     * \returns the last received telegram or an empty string if no telegram was received or something went wrong.
     */
    std::string receiveStringTelegram();
    std::vector<uint8_t> receiveTelegram();

    /** Convenience function: Send a telegram and wait for it's reply.
     *
     * \param [in] payload the actual data which is being sent to Term.
     * \param [in] message_type used internally by the DevCli dll. Depends on context. Most of the time 2.
     * \returns the last received telegram or an empty string if someting went wrong.
     *
     * \warning If the queue is not empty the last received telegram will be returned. Recommended to flush the queue first.
     * \sa clearIncomingTelegramQueue();
     */
    std::string sendStringAndWaitForReplyString(std::string payload, char message_type);

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

    SOCKET socket_handle;

    std::mutex receivedTelegramsGuard;
    std::queue< std::vector<uint8_t> > receivedTelegrams;

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
    std::vector<uint8_t> readTelegramFromSocket();

    /** Helper function getting the current time in milliseconds. */
    std::chrono::milliseconds getCurrentTimeInMilliseconds() const;

    void closeSocket();

};

#endif // THALESREMOTECONNECTION_H
