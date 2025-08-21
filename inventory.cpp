#include <iostream>
#include <string>
#include <vector>

// Global counter for tracking updates. Useful for a watchpoint.
int g_items_updated = 0;

struct Item {
    int id;
    std::string name;
    int quantity;
};

/**
 * @brief A complex, undocumented function to generate a validation code.
 * Students need to trace this to understand its logic.
 */
long generate_validation_code(int level) {
    if (level <= 0) {
        return 1;
    }
    // Recurses differently based on whether the level is even or odd.
    if (level % 2 == 1) {
        return level + generate_validation_code(level - 1);
    } else {
        return level * generate_validation_code(level - 2);
    }
}

/**
 * @brief Updates item quantities in the inventory. Contains a critical bug.
 */
void update_inventory(Item** inventory, int size) {
    std::cout << "Updating inventory quantities..." << std::endl;
    // This loop has a subtle off-by-one error.
    for (int i = 0; i <= size; ++i) {
        // This conditional is a great target for a conditional breakpoint.
        if (inventory[i]->id == 103) {
            inventory[i]->quantity -= 5;
        }
        inventory[i]->quantity++;
        g_items_updated++;
    }
}

/**
 * @brief Prints the current state of the inventory.
 */
void print_inventory(Item** inventory, int size) {
    std::cout << "\n--- Current Inventory ---" << std::endl;
    for (int i = 0; i < size; ++i) {
        if (inventory[i]) {
            std::cout << "ID: " << inventory[i]->id
                      << ", Name: " << inventory[i]->name
                      << ", Quantity: " << inventory[i]->quantity << std::endl;
        }
    }
    std::cout << "-------------------------" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Error: Missing mode. Usage: ./inventory <update|validate>" << std::endl;
        return 1;
    }

    const int INVENTORY_SIZE = 5;
    // A dynamically allocated array of pointers to Item objects.
    Item** stock = new Item*[INVENTORY_SIZE];

    stock[0] = new Item{101, "Flux Capacitor", 10};
    stock[1] = new Item{102, "Fusion Reactor", 5};
    stock[2] = new Item{103, "Plutonium Rod", 20};
    stock[3] = new Item{104, "Hoverboard", 8};
    stock[4] = new Item{105, "Time Circuit", 15};

    std::string mode = argv[1];
    std::cout << "Operating in '" << mode << "' mode." << std::endl;

    if (mode == "update") {
        update_inventory(stock, INVENTORY_SIZE);
        std::cout << "Inventory update complete." << std::endl;
        print_inventory(stock, INVENTORY_SIZE);
    } else if (mode == "validate") {
        long code = generate_validation_code(INVENTORY_SIZE);
        std::cout << "System validation code: " << code << std::endl;
    } else {
        std::cerr << "Unknown mode: " << mode << std::endl;
    }

    // Cleanup allocated memory
    for (int i = 0; i < INVENTORY_SIZE; ++i) {
        delete stock[i];
    }
    delete[] stock;

    std::cout << "Program finished." << std::endl;
    return 0;
}
