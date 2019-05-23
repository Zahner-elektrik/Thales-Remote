#include "thalesremoteconnection.h"

ThalesRemoteConnection::ThalesRemoteConnection() :
    socket_handle(-1)
{

}

bool ThalesRemoteConnection::connectToTerm(std::string address, std::string connectionName) {

    struct sockaddr_in term_address;

    if ((this->socket_handle = socket(AF_INET, SOCK_STREAM, 0)) < 0) {

            std::cout << "could not create socket" << std::endl;
            return false;
    }

    memset(&term_address, '0', sizeof(term_address));
    term_address.sin_family = AF_INET;
    term_address.sin_port = htons(260);

    if (inet_pton(AF_INET, address.c_str() , &term_address.sin_addr) <= 0) {

        std::cout << "invalid address" << std::endl;
        return false;
    }

    if (connect(this->socket_handle, (struct sockaddr *)&term_address, sizeof(term_address)) < 0) {

          std::cout << "could not connect to term" << std::endl;
          return false;
    }

    this->startTelegramListener();

    std::this_thread::sleep_for(std::chrono::milliseconds(400));

    short payloadLength = connectionName.length();
    char *payloadLengthPointer = reinterpret_cast<char*>(&payloadLength);

    char headerData[9] =    "\x00\x00"     // Length of payload string (16 bit)
                            "\x02\xd0"     // Protocol version (don't change)
                            "\xff\xff"     // Buffer size (just use 0xffff for max)
                            "\xff\xff";    // Internal protocol bytes (keep 0xffff)

    headerData[1] = payloadLengthPointer[1];
    headerData[0] = payloadLengthPointer[0];

    send(this->socket_handle, headerData, 8, MSG_MORE);
    send(this->socket_handle, connectionName.c_str(), connectionName.length(), 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(800));

    return true;
}

void ThalesRemoteConnection::disconnectFromTerm() {

    // just 0xffff on "channel" 4 is the message to disconnect for Term
    this->sendTelegram("\xff\xff", 4);

    this->stopTelegramListener();

    close(this->socket_handle);
    this->socket_handle = -1;
}

bool ThalesRemoteConnection::isConnectedToTerm() const {

    return (this->socket_handle > 0);
}

void ThalesRemoteConnection::sendTelegram(std::string payload, char message_type) {

    // The header is three bytes long:
    // - Two bytes for the length of the telegram as unsigned integer in little endian
    // - One byte is the telegram type
    char header_bytes[3];

    // Setting the length...
    unsigned short *length_data = reinterpret_cast<unsigned short*>(header_bytes);
    *length_data = payload.length();

    // ...and one byte for the message type
    header_bytes[2] = message_type;

    // First write the header into the buffer without starting the transmission
    // NOTE: Term cannot handle just getting the header and then later the remainig packet
    send(this->socket_handle, header_bytes, 3, MSG_MORE);
    send(this->socket_handle, payload.c_str(), payload.length(), 0);
}

std::string ThalesRemoteConnection::waitForTelegram() {

    while (this->telegramReceived() == false) {

        // Make sure the thread hangs here. The mutex will be unlocked upon
        // receiving the next telegram.
        this->telegramsAvailableMutex.lock();
    }

    return this->receiveTelegram();
}

std::string ThalesRemoteConnection::waitForTelegram(const std::chrono::duration<int, std::milli> timeout) {

    std::chrono::milliseconds startTime = this->getCurrentTimeInMilliseconds();
    std::chrono::duration<int, std::milli> remainingTime;
    std::chrono::duration<int, std::milli> elapsedTime;

    while (this->telegramReceived() == false) {

        elapsedTime = this->getCurrentTimeInMilliseconds() - startTime;

        // Check if the timeout has already occured
        if (elapsedTime > timeout) {

            return std::string();
        }

        remainingTime = timeout - elapsedTime;

        // This function may randomly fail (according to the documentation of std::timed_mutex).
        this->telegramsAvailableMutex.try_lock_for(remainingTime);

        // Just re-check after it failed and freze for the remaining time if needed.
    }

    // If a telegram is received while waiting it can be delivered.
    return this->receiveTelegram();
}

std::string ThalesRemoteConnection::receiveTelegram() {

    std::string receivedTelegram;

    // Making sure we won't read from the queue while the thread might be
    // in the process of putting in a new telegram.
    this->receivedTelegramsGuard.lock();

    if (this->receivedTelegrams.empty() == false) {

        receivedTelegram = this->receivedTelegrams.front();
        this->receivedTelegrams.pop();

    }

    this->receivedTelegramsGuard.unlock();

    return receivedTelegram;
}

std::string ThalesRemoteConnection::sendAndWaitForReply(std::string payload, char message_type) {

    // This is just a convenience method.
    this->sendTelegram(payload, message_type);
    return this->waitForTelegram();
}

bool ThalesRemoteConnection::telegramReceived() {

    bool telegramsAvailable = false;

    this->receivedTelegramsGuard.lock();

    telegramsAvailable = !this->receivedTelegrams.empty();

    this->receivedTelegramsGuard.unlock();

    return telegramsAvailable;
}

void ThalesRemoteConnection::clearIncomingTelegramQueue() {

    this->receivedTelegramsGuard.lock();

    while(this->receivedTelegrams.empty() == false) {
        this->receivedTelegrams.pop();
    }

    this->receivedTelegramsGuard.unlock();
}

std::string ThalesRemoteConnection::readTelegramFromSocket() {

    int received_bytes;
    int total_received_bytes = 0;
    char header_bytes[3];
    unsigned short *length_data = reinterpret_cast<unsigned short*>(header_bytes);
    std::string result;

    do {

        // Firstly we try to read the three header bytes of the telegram.
        received_bytes = recv(this->socket_handle, &header_bytes[total_received_bytes], 3 - total_received_bytes, 0);

        // Quit if the socket has been shut down.
        if (received_bytes == 0) {

            return result;
        }

        total_received_bytes += received_bytes;

    } while (total_received_bytes < 3);

    // After the length is known prepare some buffer for the incoming data.
    char *receiving_buffer = new char[*length_data + 1];

    // Add a trailing zero because the received content will be stored in a std::string.
    receiving_buffer[*length_data] = '\0';

    total_received_bytes = 0;

    do {

        received_bytes = recv(this->socket_handle, &receiving_buffer[total_received_bytes], *length_data - total_received_bytes, 0);

        if (received_bytes == 0) {

            delete receiving_buffer;
            return result;
        }

        total_received_bytes += received_bytes;

    } while (total_received_bytes < *length_data);

    result.append(receiving_buffer);

    // clean up the reserved buffer memory
    delete receiving_buffer;

    return result;
}

void ThalesRemoteConnection::telegramListenerJob() {

    do {

        // Most of the time the thread will be blocking here
        std::string telegram = readTelegramFromSocket();

        if (telegram.length() > 0) {

            this->receivedTelegramsGuard.lock();

            this->receivedTelegrams.push(telegram);

            // unlock the mutex in case the client thread is
            // blocking while waiting for an incoming telegram
            this->telegramsAvailableMutex.unlock();

            this->receivedTelegramsGuard.unlock();

        }

    } while (this->receiving_worker_is_running);
}

void ThalesRemoteConnection::startTelegramListener() {

    this->receiving_worker_is_running = true;
    this->telegramsAvailableMutex.lock();
    this->receivingWorker = new std::thread(&ThalesRemoteConnection::telegramListenerJob, this);
}

void ThalesRemoteConnection::stopTelegramListener() {

    // Makes sure the blocking recv function returns so the thread
    // can be shut down gracefully.
    shutdown(this->socket_handle, SHUT_RD);

    this->receiving_worker_is_running = false;
    this->receivingWorker->join();
}

std::chrono::milliseconds ThalesRemoteConnection::getCurrentTimeInMilliseconds() const {

    return std::chrono::duration_cast<std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch());
}
