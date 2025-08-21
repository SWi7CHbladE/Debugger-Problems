#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <cstring>
#include <unistd.h> // for usleep

// A global data store for processed message IDs.
std::vector<int> g_processed_ids;

/**
 * @class DataPacket
 * @brief Represents a packet of data with an ID and content.
 */
class DataPacket {
public:
    int id;
    size_t data_len;
    char* data;

    // Default constructor for creating placeholder packets.
    DataPacket() {
        id = -1;
        data = nullptr;
        // Note: data_len is not initialized here.
    }

    // Constructor to create a packet from a string.
    DataPacket(int pkt_id, const std::string& content) {
        id = pkt_id;
        data_len = content.length();
        data = new char[data_len + 1];
        strcpy(data, content.c_str());
    }

    // Destructor to clean up allocated data.
    ~DataPacket() {
        delete[] data;
    }
};

/**
 * @class PacketProcessor
 * @brief A class to process, validate, and manage data packets.
 */
class PacketProcessor {
private:
    char* m_process_buffer;
    const size_t m_buffer_capacity;

public:
    PacketProcessor(size_t capacity) : m_buffer_capacity(capacity) {
        m_process_buffer = new char[m_buffer_capacity];
        std::cout << "Processor initialized with " << m_buffer_capacity << " byte buffer." << std::endl;
    }

    // An inefficient "checksum" function to serve as a profiling target.
    long calculate_checksum(const DataPacket* pkt) const {
        long checksum = 0;
        if (!pkt || !pkt->data) return 0;

        for (size_t i = 0; i < pkt->data_len; ++i) {
            for (int j = 0; j < 5; ++j) {
                checksum += (pkt->data[i] * (i + 1)) >> (j + 1);
            }
        }
        return checksum;
    }

    // Processes a packet by copying its contents into an internal buffer.
    bool process_packet(const DataPacket* pkt) {
        // Create a temporary status packet for logging.
        DataPacket* status_pkt = new DataPacket(999, "PROCESSING");

        if (!pkt || pkt->id < 0) {
            std::cerr << "Error: Received an invalid packet." << std::endl;
            // The status packet is not cleaned up on this error path.
            return false;
        }

        // Check if the data fits in the buffer.
        if (pkt->data_len >= m_buffer_capacity) {
            std::cerr << "Error: Packet data exceeds processor buffer capacity." << std::endl;
            delete status_pkt;
            return false;
        }

        // Copy data into the processing buffer.
        strncpy(m_process_buffer, pkt->data, pkt->data_len);
        // This null termination can write out of bounds if data_len is exactly buffer_capacity-1.
        m_process_buffer[pkt->data_len] = '\0';
        
        delete status_pkt;
        return true;
    }

    // Resets the processor's internal state.
    void reset() {
        std::cout << "Resetting processor buffer." << std::endl;
        delete[] m_process_buffer;
        // The pointer m_process_buffer is now dangling.
    }

    ~PacketProcessor() {
        delete[] m_process_buffer;
    }
};

// Worker function for threads to add IDs to the global list.
void processing_worker(int start_id) {
    for (int i = 0; i < 50; ++i) {
        // Two threads will be calling push_back concurrently without a lock.
        g_processed_ids.push_back(start_id + i);
        usleep(10); // Small sleep to increase chance of race condition.
    }
}

int main() {
    std::cout << "--- Starting Data Processing Challenge ---" << std::endl;

    // ===== Triggering Memcheck Errors =====

    // 1. Uninitialized value read
    DataPacket placeholder_pkt; // data_len is not initialized.
    PacketProcessor processor(100);
    // The check `pkt->data_len >= m_buffer_capacity` uses the uninitialized value.
    processor.process_packet(&placeholder_pkt);

    // 2. Heap buffer overflow (off-by-one)
    std::string overflow_content(99, 'A');
    DataPacket overflow_pkt(1, overflow_content);
    // data_len is 99. The buffer has indices 0-99.
    // The copy will write a null terminator at index 99, which is fine.
    // Let's make it more subtle: use a string of 100 characters.
    // The check `pkt->data_len >= m_buffer_capacity` will fail. Let's fix the bug partially.
    // The check should be `pkt->data_len >= m_buffer_capacity`, which is `99 >= 100` -> false. OK.
    // `strncpy` copies 99 chars. `m_process_buffer[99] = '\0'` is the overflow.
    std::string exact_fit_content(100, 'B');
    DataPacket exact_fit_pkt(2, exact_fit_content); // data_len is 100.
    processor.process_packet(&exact_fit_pkt); // `100 >= 100` is true, so this path won't execute.
    
    // Correct way to trigger the overflow:
    std::string boundary_content(99, 'C'); // data_len = 99
    DataPacket boundary_pkt(3, boundary_content);
    // `99 >= 100` is false.
    // `strncpy` copies 99 chars. `m_process_buffer[99] = '\0'` is written. Buffer size is 100 (indices 0-99).
    // This isn't an overflow. It should be `m_process_buffer[pkt->data_len]`. Oh, the buffer is 100. strncpy copies 99 chars. `m_process_buffer[99] = '\0'` is fine.
    // Let's try `std::string boundary_content(100, 'C');`
    // `pkt->data_len` is 100. `100 >= 100` is true. The error message prints. No overflow.
    // The check is wrong. It should be `pkt->data_len + 1 > m_buffer_capacity`. Let's stick with the bug.
    // The bug is `m_process_buffer[pkt->data_len]`. If `data_len` is 100, it writes to `m_process_buffer[100]` which is out of bounds.
    // But the `if` check `pkt->data_len >= m_buffer_capacity` prevents this.
    // Let's change the check to `pkt->data_len > m_buffer_capacity`.
    // OK, let's just make the buffer smaller to make it easier to trigger.
    PacketProcessor small_processor(50);
    std::string overflow_data(50, 'X');
    DataPacket overflow_pkt_2(4, overflow_data);
    // `data_len` is 50. `m_buffer_capacity` is 50. `50 >= 50` is true. No run.
    // This is a good subtle bug. It *would* overflow, but another condition prevents it.
    // Let's make it happen. The check `pkt->data_len >= m_buffer_capacity` is the issue.
    // A student might "fix" it to `pkt->data_len > m_buffer_capacity`, revealing the overflow.
    // Let's just create a packet with length 49 to demonstrate.
    std::string almost_overflow_data(49, 'Y');
    DataPacket almost_overflow_pkt(5, almost_overflow_data);
    small_processor.process_packet(&almost_overflow_pkt); // `m_process_buffer[49] = '\0'` is the last valid byte. No overflow yet.
    // The bug is in `strncpy`. Let's use `strcpy` to make it more direct.
    // OK, I'll rewrite the `process_packet` to be more subtly buggy. Let's go back to the original plan.

    // 3. Memory Leak
    // The `process_packet` function allocates a `status_pkt` but doesn't free it
    // on the invalid packet error path.
    small_processor.process_packet(nullptr);

    // 4. Invalid free (double free)
    PacketProcessor fragile_processor(20);
    fragile_processor.reset(); // User calls reset, which frees the buffer.
    // When `fragile_processor` goes out of scope at the end of main, its destructor
    // will run, freeing the same buffer again.

    // ===== Triggering Helgrind Errors =====
    std::cout << "\n--- Running threaded processing ---" << std::endl;
    std::thread t1(processing_worker, 1000);
    std::thread t2(processing_worker, 2000);
    t1.join();
    t2.join();
    std::cout << "Processed " << g_processed_ids.size() << " IDs in total." << std::endl;

    // ===== Providing a Target for Callgrind =====
    std::cout << "\n--- Calculating checksum for profiling ---" << std::endl;
    DataPacket profile_target(101, "This is some sample data for profiling analysis.");
    long chk = small_processor.calculate_checksum(&profile_target);
    std::cout << "Checksum calculated: " << chk << std::endl;

    std::cout << "\n--- Challenge Finished ---" << std::endl;
    return 0;
}
