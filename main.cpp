//
//  main.cpp
//  pds
//
//  Created by Arshia Taghavinejad on 2023-12-20.
//
#include <string>
#include <map>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <exception>
#include <unordered_set>
#include <mach/mach.h>
#include <cassert>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <numeric>
using namespace std;

string simpleHash(const string& password) {
    // A simple hash function for demonstration. Do not use in production!
    hash<string> hasher;
    auto hashed = hasher(password);
    stringstream ss;
    ss << hashed;
    return ss.str();
}
struct SystemData {
    // Example fields
    double cpuUsage;
    double memoryUsage;
    long requestCount;
    // ... other relevant fields

    // Convert data to a CSV string
    string toString() const {
        stringstream ss;
        ss << cpuUsage << "," << memoryUsage << "," << requestCount; // Add other fields as needed
        return ss.str();
    }
};
void collectAndSaveData() {
    // Collect data
    SystemData data;
    // TODO: Fill data with actual metrics (e.g., CPU usage, memory usage, etc.)

    // Save data to a file
    std::ofstream file("system_data.csv", std::ios::app); // Append mode
    if (file.is_open()) {
        file << data.toString() << std::endl; // Format data as CSV
        file.close();
    } else {
        std::cerr << "Unable to open file for writing." << std::endl;
    }
}



class Role {
public:
    Role() = default;
    string name;
    unordered_set<string> permissions;

    Role(const string& name, const unordered_set<string>& permissions)
        : name(name), permissions(permissions) {}
};

class User {
public:
    User() = default;
    string username;
    string password; // Should be hashed in a real system
    string role;

    User(const string& username, const string& password, const string& role)
        : username(username), password(password), role(role) {}
};

class Service; // Forward declaration

class ServiceDiscovery {
public:
    virtual vector<shared_ptr<Service>> discoverServices() = 0;
};

class SecurityManager {
private:
    unordered_map<string, User> users;
    unordered_map<string, Role> roles;
    unordered_map<string, string> pendingUsers; // Users awaiting admin approval
    
    string serialize() {
            stringstream ss;
            // Serialize users
            for (const auto& [username, user] : users) {
                ss << username << "," << user.password << "," << user.role << ";";
            }
            ss << endl;
            // Serialize roles
            for (const auto& [roleName, role] : roles) {
                ss << roleName << ":";
                for (const auto& perm : role.permissions) {
                    ss << perm << ",";
                }
                ss << ";";
            }
            return ss.str();
        }

        // Deserializes a string to users and roles
        void deserialize(const string& data) {
            stringstream ss(data);
            string usersPart, rolesPart;
            getline(ss, usersPart, '\n');
            getline(ss, rolesPart, '\n');
            
            // Deserialize users
            stringstream usersStream(usersPart);
            string userEntry;
            while (getline(usersStream, userEntry, ';')) {
                stringstream userStream(userEntry);
                string username, password, role;
                getline(userStream, username, ',');
                getline(userStream, password, ',');
                getline(userStream, role, ',');
                users[username] = User(username, password, role);
            }
            
            // Deserialize roles
            stringstream rolesStream(rolesPart);
            string roleEntry;
            while (getline(rolesStream, roleEntry, ';')) {
                stringstream roleStream(roleEntry);
                string roleName, perm;
                getline(roleStream, roleName, ':');
                unordered_set<string> permissions;
                while (getline(roleStream, perm, ',')) {
                    if (!perm.empty()) {
                        permissions.insert(perm);
                    }
                }
                roles[roleName] = Role(roleName, permissions);
            }
        }
    
    void initializeDefaultData() {
            // Define default roles and admin user
            roles["admin"] = Role("admin", {"read", "write", "modify", "approveUser", "assignRole"});
            roles["user"] = Role("user", {"read"});
            roles["guest"] = Role("guest", {});
            users["admin"] = User("admin", simpleHash("adminPass"), "admin");
            
            // Save to file
            saveToFile("security_data.txt");
        }
    
public:
    
    
    vector<string> getPendingUsers() {
           vector<string> pendingUsernames;
           for (const auto& user : pendingUsers) {
               pendingUsernames.push_back(user.first);
           }
           return pendingUsernames;
       }

       // Retrieve all roles
       vector<string> getAllRoles() {
           vector<string> roleNames;
           for (const auto& role : roles) {
               roleNames.push_back(role.first);
           }
           return roleNames;
       }

       // Retrieve a specific user's role
       string getUserRole(const string& username) {
           if (users.find(username) != users.end()) {
               return users[username].role;
           }
           return "User not found";
       }

       // Retrieve all approved users with their roles
       vector<pair<string, string>> getAllUsersWithRoles() {
           vector<pair<string, string>> userDetails;
           for (const auto& user : users) {
               userDetails.push_back({user.first, getUserRole(user.first)});
           }
           return userDetails;
       }
    
    void saveToFile(const string& filename) {
            ofstream file(filename);
            if (file.is_open()) {
                file << serialize();
                file.close();
                cout << "Data saved to " << filename << endl;
            } else {
                cout << "Unable to open file for writing." << endl;
            }
        }

        // Loads the state from a file
        void loadFromFile(const string& filename) {
            ifstream file(filename);
            if (file.is_open()) {
                stringstream buffer;
                buffer << file.rdbuf();
                file.close();
                deserialize(buffer.str());
                cout << "Data loaded from " << filename << endl;
            } else {
                cout << "Unable to open file for reading." << endl;
            }
        }
    
    SecurityManager() {
        // Load data from file or initialize default data if the file doesn't exist
               ifstream file("security_data.txt");
               if (file.good()) {
                   // File exists, load data
                   loadFromFile("security_data.txt");
               } else {
                   // File doesn't exist, initialize default data
                   initializeDefaultData();
               }
        }

    bool hasAccess(const string& username, const string& component, const string& action) {
        // Check if the user exists
              if (users.find(username) == users.end()) {
                  return false; // User does not exist
              }

              // Get the user's role
              string role = users[username].role;

              // Check if the role has the required permission for the action
              // Here, you might need to map the action and component to specific permissions
              string requiredPermission = component + "_" + action; // Example format: "ComponentName_action"
              return roles[role].permissions.find(requiredPermission) != roles[role].permissions.end();
          }
        
    
    void createRole(const string& roleName, const unordered_set<string>& permissions) {
            roles[roleName] = Role(roleName, permissions);
        }

        void modifyRole(const string& roleName, const unordered_set<string>& newPermissions) {
            if (roles.find(roleName) != roles.end()) {
                roles[roleName].permissions = newPermissions;
            }
        }

        void deleteRole(const string& roleName) {
            roles.erase(roleName);
        }
    
    void assignRoleToUser(const string& username, const string& roleName) {
           if (users.find(username) != users.end() && roles.find(roleName) != roles.end()) {
               users[username].role = roleName;
           }
       }
    
    bool authorize(const string& username, const string& permission) {
           if (users.find(username) != users.end()) {
               string role = users[username].role;
               return roles[role].permissions.find(permission) != roles[role].permissions.end();
           }
           return false;
       }

    bool authenticate(const std::string& username, const std::string& password) {
        if (users.find(username) != users.end()) {
            std::string hashedPassword = simpleHash(password);
            if (users[username].password == hashedPassword) {
                std::cout << "Authentication successful for user: " << username << std::endl;
                return true;
            }
        }
        std::cout << "Authentication failed for user: " << username << std::endl;
        return false;
    }
    void userSignup(const std::string& username, const std::string& password) {
        if (users.find(username) == users.end()) {
            std::string hashedPassword = simpleHash(password);
            pendingUsers[username] = hashedPassword;
            std::cout << "Signup request for user " << username << " is pending approval." << std::endl;
        } else {
            std::cout << "Username " << username << " already exists." << std::endl;
        }
    }
        void approveUser(const string& adminUsername, const string& username) {
            if (isAdmin(adminUsername) && pendingUsers.find(username) != pendingUsers.end()) {
                users[username] = User(username, pendingUsers[username], "guest"); // Default to "guest" or another role
                pendingUsers.erase(username);
                cout << "User " << username << " approved and assigned default role." << endl;
            } else {
                cout << "Approval failed. Either user doesn't exist or you're not authorized." << endl;
            }
        }

        void assignRole(const string& adminUsername, const string& username, const string& newRole) {
            if (isAdmin(adminUsername)) {
                if (users.find(username) != users.end() && roles.find(newRole) != roles.end()) {
                    users[username].role = newRole;
                    cout << "Role " << newRole << " assigned to user " << username << endl;
                } else {
                    cout << "User or role not found." << endl;
                }
            } else {
                cout << "Only admin can assign roles." << endl;
            }
        }

        // Helper method to check if a user is an admin
        bool isAdmin(const string& username) {
            return users.find(username) != users.end() && users[username].role == "admin";
        }


   
};


class ConflictException : public exception {
public:
    const char* what() const throw () override {
        return "Service conflict detected!";
    }
};

class DuplicateComponentException : public exception {
public:
    const char* what() const throw () override {
        return "Duplicate component detected!";
    }
    
    
};

class Service {
public:
    string serviceName;
    int version;
    map<int, string> versionHistory;
    
    Service(const string& serviceName, int version) : serviceName(serviceName), version(version) {}
    
    void addToHistory(int version, const string& description) {
        versionHistory[version] = description;
    }
   
    bool rollback(int version) {
           if (versionHistory.find(version) != versionHistory.end()) {
               this->version = version;
               // Additional rollback logic...
               return true;
           }
           return false;
       }
   
    
};


class Component {
public:
    string name;
        vector<shared_ptr<Service>> imports;
        vector<shared_ptr<Service>> exports;

        // Default constructor
        Component() = default;

        // Custom constructor
        Component(const string& name) : name(name) {}

        // Copy constructor
        Component(const Component& other) : name(other.name), imports(other.imports), exports(other.exports) {}

        // Move constructor
    Component(Component&& other) noexcept : name(std::move(other.name)), imports(std::move(other.imports)), exports(std::move(other.exports)) {}

        // Copy assignment operator
        Component& operator=(const Component& other) {
            if (this != &other) {
                name = other.name;
                imports = other.imports;
                exports = other.exports;
            }
            return *this;
        }

        // Move assignment operator
        Component& operator=(Component&& other) noexcept {
            if (this != &other) {
                name = std::move(other.name);
                imports = std::move(other.imports);
                exports = std::move(other.exports);
            }
            return *this;
        }


    void onServiceUpdate(const string& serviceName, int newVersion) {
        cout << "Component '" << name << "' notified: Service '"
        << serviceName << "' updated to version " << newVersion << endl;
       }
    
    void addImport(shared_ptr<Service> service) {
        auto it = find(exports.begin(), exports.end(), service);
        if (it == exports.end()) {
            imports.push_back(service);
        } else {
            throw ConflictException();
        }
    }

    void addExport(shared_ptr<Service> service) {
        auto it = find(imports.begin(), imports.end(), service);
        if (it == imports.end()) {
            exports.push_back(service);
        } else {
            throw ConflictException();
        }
    }
};


class AuditLog {
public:
    static void logAction(const string& username, const string& action, const string& details) {
           // Get the current timestamp
           auto now = std::chrono::system_clock::now();
           auto now_c = std::chrono::system_clock::to_time_t(now);
           std::stringstream ss;
           ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %X"); // Format: YYYY-MM-DD HH:MM:SS

           // Open log file in append mode
           std::ofstream logFile("action_log.txt", std::ios::app);
           if (logFile.is_open()) {
               // Write the log entry
               logFile << "[" << ss.str() << "] " << username << " performed '" << action << "' on " << details << "\n";
               logFile.close();
           } else {
               std::cerr << "Unable to open log file." << std::endl;
           }
       }
};

// Ensure `AuditLog` is used in relevant parts of your code, such as in `SecurityManager` methods


class SecureCommunicator {
public:
    void setupTLS() {
        // In an actual implementation, this would configure SSL/TLS settings.
        cout << "Setting up TLS (simulated)" << endl;
    }

    string encryptData(const string& data) {
        string encrypted = data;
        transform(encrypted.begin(), encrypted.end(), encrypted.begin(), [](char c) { return c + 1; });
        reverse(encrypted.begin(), encrypted.end());
        return encrypted;
    }

    string decryptData(const string& data) {
        string decrypted = data;
        reverse(decrypted.begin(), decrypted.end());
        transform(decrypted.begin(), decrypted.end(), decrypted.begin(), [](char c) { return c - 1; });
        return decrypted;
    }
};

class PDS {
private:
    
    unordered_map<string, Component> components;
    unordered_map<string, Component> previousStates;
    bool dfs(const string& componentName, unordered_set<string>& visited, unordered_set<string>& recStack) {
            if (visited.find(componentName) == visited.end()) {
                visited.insert(componentName);
                recStack.insert(componentName);

                const auto& comp = components[componentName];
                for (const auto& service : comp.imports) {
                    // Find component exporting this service
                    auto exporter = find_if(components.begin(), components.end(),
                        [&service](const auto& pair) {
                            return find_if(pair.second.exports.begin(), pair.second.exports.end(),
                                [&service](const shared_ptr<Service>& s) { return s->serviceName == service->serviceName; }) != pair.second.exports.end();
                        });

                    if (exporter != components.end()) {
                        const string& exporterName = exporter->first;
                        if (visited.find(exporterName) == visited.end() && dfs(exporterName, visited, recStack)) {
                            return true;
                        } else if (recStack.find(exporterName) != recStack.end()) {
                            return true;
                        }
                    }
                }
            }
            recStack.erase(componentName);
            return false;
        }
    
    bool checkEachComponentHasImportsAndExports() const {
           for (const auto& pair : components) {
               const auto& comp = pair.second;
               if (comp.imports.empty() || comp.exports.empty()) {
                   return false;
               }
           }
           return true;
       }
    
    bool checkForOrphanedServices() const {
            for (const auto& exportPair : components) {
                for (const auto& exportService : exportPair.second.exports) {
                    bool isImported = false;
                    for (const auto& importPair : components) {
                        if (importPair.first != exportPair.first) { // Skipping self
                            for (const auto& importService : importPair.second.imports) {
                                if (importService->serviceName == exportService->serviceName) {
                                    isImported = true;
                                    break;
                                }
                            }
                        }
                        if (isImported) break;
                    }
                    if (!isImported) return false;
                }
            }
            return true;
        }

    string getCurrentMemoryUsage() {
            mach_task_basic_info info;
            mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;

            if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                          (task_info_t)&info, &infoCount) != KERN_SUCCESS) {
                return "Error retrieving memory info";
            }

            // Convert bytes to kilobytes
            unsigned long memorySizeKB = info.resident_size / 1024;
            return to_string(memorySizeKB) + " KB";
        }

public:
    
    void reportResourceUsage() {
            string memoryUsage = getCurrentMemoryUsage();
            cout << "Current Memory Usage: " << memoryUsage << endl;
        }

    void addComponent(const Component& component) {
        if (components.find(component.name) != components.end()) {
            throw DuplicateComponentException();
        }
        components[component.name] = component;
    }


    bool removeComponent(const string& componentName) {
        if (components.erase(componentName) == 0) {
            return false;
        }
        return true;
    }

    bool areDependenciesResolved() const {
        for (const auto& pair : components) {
            const auto& component = pair.second;
            for (const auto& service : component.imports) {
                bool isResolved = any_of(components.begin(), components.end(),
                    [&service, &component](const auto& p) {
                        const auto& c = p.second;
                        return find_if(c.exports.begin(), c.exports.end(),
                            [&service](const shared_ptr<Service>& s) { return s == service; }) != c.exports.end()
                            && c.name != component.name;
                    });

                if (!isResolved) return false;
            }
        }
        return true;
    }

    Component* getComponent(const string& componentName) {
        auto it = components.find(componentName);
        if (it != components.end()) {
            return &(it->second);
        }
        return nullptr;
    }

    vector<string> listAllComponents() const {
        vector<string> componentNames;
        for (const auto& pair : components) {
            componentNames.push_back(pair.first);
        }
        return componentNames;
    }

    void displayComponentDetails(const string& componentName) const {
        const auto& it = components.find(componentName);
        if (it != components.end()) {
            const auto& comp = it->second;
            cout << "Component Name: " << comp.name << "\n";
            cout << "Imports: ";
            for (const auto& imp : comp.imports) {
                cout << imp->serviceName << " (v" << imp->version << ") ";
            }
            cout << "\nExports: ";
            for (const auto& exp : comp.exports) {
                cout << exp->serviceName << " (v" << exp->version << ") ";
            }
            cout << "\n";
        } else {
            cout << "Component not found." << "\n";
        }
    }

    int HighestVersionPolicy(const string& serviceName) const {
        int highestVersion = -1;
        for (const auto& pair : components) {
            const auto& comp = pair.second;
            for (const auto& service : comp.exports) {
                if (service->serviceName == serviceName && service->version > highestVersion) {
                    highestVersion = service->version;
                }
            }
        }
        return highestVersion;
    }

    bool updateComponent(const string& componentName, const Component& updatedComponent) {
        auto it = components.find(componentName);
        if (it != components.end()) {
            if (componentName == updatedComponent.name || components.find(updatedComponent.name) == components.end()) {
                it->second = updatedComponent;
                return true;
            }
        }
        return false;
    }

    bool checkSystemCompatibility() const {
        return true;
    }

    void generateSystemReport() const {
        cout << "PDS System Report\n-----------------\n";
        cout << "Total Components: " << components.size() << "\n";
        for (const auto& pair : components) {
            displayComponentDetails(pair.first);
            cout << "-----------------\n";
        }
    }
    
    bool updateComponentWithBackup(const string& componentName, const Component& updatedComponent) {
            auto it = components.find(componentName);
            if (it != components.end()) {
                if (componentName == updatedComponent.name || components.find(updatedComponent.name) == components.end()) {
                    // Backup current state before update
                    previousStates[componentName] = it->second;
                    it->second = updatedComponent;
                    return true;
                }
            }
            return false;
        }
    
    bool rollbackComponentUpdate(const string& componentName) {
            if (previousStates.find(componentName) != previousStates.end()) {
                components[componentName] = previousStates[componentName];
                previousStates.erase(componentName);
                return true;
            }
            return false; // No previous state to rollback to
        }
    
    void updateServiceVersion(const string& serviceName, int newVersion) {
        // Update the export version in the component that exports the service
        for (auto& pair : components) {
            auto& comp = pair.second;
            for (auto& service : comp.exports) {
                if (service->serviceName == serviceName) {
                    service->version = newVersion;
                }
            }
        }

        // Update the import version in all components that import the service
        for (auto& pair : components) {
            auto& comp = pair.second;
            for (auto& service : comp.imports) {
                if (service->serviceName == serviceName) {
                    service->version = newVersion;
                }
            }
        }
    }


    vector<string> findComponentsByService(const string& serviceName) {
           vector<string> foundComponents;
           for (const auto& pair : components) {
               const auto& comp = pair.second;
               auto it = find_if(comp.exports.begin(), comp.exports.end(),
                                 [&serviceName](const shared_ptr<Service>& service) { return service->serviceName == serviceName; });
               if (it != comp.exports.end()) {
                   foundComponents.push_back(comp.name);
               }
           }
           return foundComponents;
       }

    void deactivateService(const string& serviceName) {
           for (auto& pair : components) {
               auto& comp = pair.second;
               comp.exports.erase(
                   remove_if(comp.exports.begin(), comp.exports.end(),
                             [&serviceName](const shared_ptr<Service>& service) { return service->serviceName == serviceName; }),
                   comp.exports.end()
               );
           }
       }
    
    void displayServicesWithVersion() {
           unordered_map<string, int> serviceVersions;
           for (const auto& pair : components) {
               for (const auto& service : pair.second.exports) {
                   serviceVersions[service->serviceName] = service->version;
               }
           }

           for (const auto& sv : serviceVersions) {
               cout << "Service: " << sv.first << ", Version: " << sv.second << "\n";
           }
       }

    vector<string> listImportsByComponent(const string& componentName) {
           vector<string> importsList;
           auto it = components.find(componentName);
           if (it != components.end()) {
               for (const auto& service : it->second.imports) {
                   importsList.push_back(service->serviceName + " (v" + to_string(service->version) + ")");
               }
           }
           return importsList;
       }
    
    bool validateExportsHaveImports() {
            for (const auto& exportPair : components) {
                for (const auto& exportService : exportPair.second.exports) {
                    bool isImported = false;
                    for (const auto& importPair : components) {
                        if (importPair.first != exportPair.first) { // Skipping self
                            for (const auto& importService : importPair.second.imports) {
                                if (importService == exportService) {
                                    isImported = true;
                                    break;
                                }
                            }
                        }
                        if (isImported) break;
                    }
                    if (!isImported) return false;
                }
            }
            return true;
        }

    void simulateServiceUpdate(const string& serviceName, int newVersion) {
           for (auto& pair : components) {
               for (auto& service : pair.second.exports) {
                   if (service->serviceName == serviceName) {
                       service = make_shared<Service>(serviceName, newVersion);
                   }
               }
           }
       }
    
    unordered_map<string, unordered_map<int, int>> getServiceSummary() {
            unordered_map<string, unordered_map<int, int>> summary;
            for (const auto& pair : components) {
                for (const auto& service : pair.second.exports) {
                    summary[service->serviceName][service->version]++;
                }
            }
            return summary;
        }

    vector<string> identifyServiceBottlenecks() {
            unordered_map<string, int> usageCount;
            for (const auto& pair : components) {
                for (const auto& service : pair.second.imports) {
                    usageCount[service->serviceName]++;
                }
            }

            vector<string> bottlenecks;
            for (const auto& pair : usageCount) {
                if (pair.second > 5) {
                    bottlenecks.push_back(pair.first);
                }
            }
            return bottlenecks;
        }

    bool performSystemHealthCheck() {
        // Check if all dependencies are resolved
        if (!areDependenciesResolved()) {
            return false;
        }

        // Additional health checks can be added here
        // Example: Check for version conflicts
        for (const auto& pair : components) {
            const auto& comp = pair.second;
            for (const auto& service : comp.imports) {
                auto it = find_if(comp.exports.begin(), comp.exports.end(),
                    [&service](const shared_ptr<Service>& exportedService) {
                        return exportedService->serviceName == service->serviceName &&
                               exportedService->version != service->version;
                    });
                if (it != comp.exports.end()) {
                    // Version conflict found
                    return false;
                }
            }
        }

        // Other health checks like resource usage, service availability, etc., can be included

        return true; // System is healthy if all checks pass
    }
    
    void simulateSystemChange(const string& serviceName) {
        cout << "Simulating change for Service: " << serviceName << "\n";

        // Count the number of components that will be affected by the change
        int affectedComponentsCount = 0;

        // Check impact on exports
        for (const auto& pair : components) {
            const auto& comp = pair.second;
            auto it = find_if(comp.exports.begin(), comp.exports.end(),
                              [&serviceName](const shared_ptr<Service>& s) { return s->serviceName == serviceName; });
            if (it != comp.exports.end()) {
                affectedComponentsCount++;
                cout << "Component " << comp.name << " will be affected in its exports.\n";
            }
        }

        // Check impact on imports
        for (const auto& pair : components) {
            const auto& comp = pair.second;
            auto it = find_if(comp.imports.begin(), comp.imports.end(),
                              [&serviceName](const shared_ptr<Service>& s) { return s->serviceName == serviceName; });
            if (it != comp.imports.end()) {
                affectedComponentsCount++;
                cout << "Component " << comp.name << " will be affected in its imports.\n";
            }
        }

        if (affectedComponentsCount == 0) {
            cout << "No components will be affected by this change.\n";
        } else {
            cout << "Total components affected: " << affectedComponentsCount << "\n";
        }
    }

    void balanceServiceLoad() {
        unordered_map<string, int> serviceLoad;
        // Calculate the load (number of imports) for each service
        for (const auto& pair : components) {
            for (const auto& service : pair.second.imports) {
                serviceLoad[service->serviceName]++;
            }
        }

        // Identify services with high load and suggest load balancing measures
        for (const auto& load : serviceLoad) {
            if (load.second > 10) {
                cout << "Service " << load.first << " is highly loaded with " << load.second << " imports. Consider load balancing.\n";
            }
        }
    }
    
    void automateServiceUpdates() {
        unordered_map<string, int> latestVersions; // Assume this map is filled with the latest versions of services

        // Iterate through each component and update its services to the latest versions
        for (auto& pair : components) {
            for (auto& service : pair.second.exports) {
                if (latestVersions.find(service->serviceName) != latestVersions.end()) {
                    int latestVersion = latestVersions[service->serviceName];
                    if (service->version < latestVersion) {
                        service->version = latestVersion; // Update to latest version
                        cout << "Service " << service->serviceName << " updated to version " << latestVersion << " in component " << pair.first << "\n";
                    }
                }
            }
        }
    }
    bool hasCircularDependencies() {
            unordered_set<string> visited, recStack;
            for (const auto& pair : components) {
                if (dfs(pair.first, visited, recStack)) {
                    return true; // Cycle detected
                }
            }
            return false;
        }
    
    void notifyServiceUpdates(const string& serviceName, int newVersion) {
          for (auto& pair : components) {
              auto& component = pair.second;
              auto it = find_if(component.imports.begin(), component.imports.end(),
                                [&serviceName](const shared_ptr<Service>& service) {
                                    return service->serviceName == serviceName;
                                });

              if (it != component.imports.end()) {
                  component.onServiceUpdate(serviceName, newVersion);
              }
          }
      }
    
    void startTransaction() {
            // Backup current state
            for (auto& pair : components) {
                previousStates[pair.first] = pair.second;
            }
        }
    
    void commitTransaction() {
            previousStates.clear(); // Clear backup after successful commit
        }
    
    void rollbackSystem() {
            if (!previousStates.empty()) {
                components = previousStates; // Revert to previous state
                previousStates.clear();
                cout << "System rollback successful." << endl;
            } else {
                cout << "No previous system state to rollback to." << endl;
            }
        }

    string getComponentDetailsAPI(const string& componentName) {
            auto comp = getComponent(componentName);
            if (comp) {
                // Convert component details to a string (or JSON-like format)
                string details = "Component Name: " + comp->name + "\n";
                // Add more details as needed
                return details;
            }
            return "Component not found";
        }
    
    string addComponentAPI(const string& componentName) {
            try {
                Component newComponent(componentName);
                addComponent(newComponent);
                return "Component added successfully";
            } catch (const exception& e) {
                return "Error: " + string(e.what());
            }
        }
    
    string removeComponentAPI(const string& componentName) {
            if (removeComponent(componentName)) {
                return "Component removed successfully";
            } else {
                return "Error removing component";
            }
        }
    
    void autoRegisterServices(ServiceDiscovery& discovery) {
           vector<shared_ptr<Service>> services = discovery.discoverServices();
           for (const auto& service : services) {
               
           }
       }

    void registerDiscoveredService(const shared_ptr<Service>& service) {
          // Check if the service is already registered
          bool serviceExists = false;
          string existingComponentName;

          for (auto& pair : components) {
              auto& component = pair.second;
              auto it = find_if(component.exports.begin(), component.exports.end(),
                                [&service](const shared_ptr<Service>& s) { return s->serviceName == service->serviceName; });
              if (it != component.exports.end()) {
                  serviceExists = true;
                  existingComponentName = pair.first;
                  break;
              }
          }

          if (serviceExists) {
              // Update the existing component with the new service version
              components[existingComponentName].addExport(service);
              cout << "Updated service '" << service->serviceName << "' in component '" << existingComponentName << "'." << endl;
          } else {
              // Create a new component for the discovered service
              string componentName = service->serviceName + "Component"; // Generate a component name based on the service name
              Component newComponent(componentName);
              newComponent.addExport(service);
              components[componentName] = newComponent;
              cout << "Created new component '" << componentName << "' for service '" << service->serviceName << "'." << endl;
          }
      }

};
class PDS_CLI {
private:
    PDS pds;
    SecurityManager securityManager;
    string currentUser;
    string currentRole;

    void mainMenu() {
        cout << "\n--- PDS Management System ---\n";
        cout << "1. Sign In\n";
        cout << "2. Sign Up\n";
        cout << "3. Exit\n";
        cout << "Select an option: ";
    }

    void adminMenu() {
        cout << "\n--- Admin Panel ---\n";
        cout << "1. Approve User\n";
        cout << "2. Assign Role to User\n";
        cout << "3. View All Users\n";
        cout << "4. Add Component\n";
        cout << "5. Remove Component\n";
        cout << "6. Return to Main Menu\n";
        cout << "Select an option: ";
    }

    void userMenu() {
        cout << "\n--- User Panel ---\n";
        cout << "1. View Components\n";
        cout << "2. Generate System Report\n";
        cout << "3. Return to Main Menu\n";
        cout << "Select an option: ";
    }

    void signIn() {
        string username, password;
        cout << "Username: ";
        cin >> username;
        cout << "Password: ";
        cin >> password;
        if (securityManager.authenticate(username, password)) {
            currentUser = username;
            currentRole = securityManager.getUserRole(username);
            cout << "Login successful!\n";
        } else {
            cout << "Login failed. Please try again.\n";
        }
    }

    void signUp() {
        string username, password;
        cout << "Choose a username: ";
        cin >> username;
        cout << "Choose a password: ";
        cin >> password;
        securityManager.userSignup(username, password);
        cout << "Signup request sent. Please wait for approval from an admin.\n";
    }

    void approveUser() {
        auto pendingUsers = securityManager.getPendingUsers();
        if (pendingUsers.empty()) {
            cout << "No pending users to approve.\n";
            return;
        }

        cout << "Pending Users:\n";
        for (int i = 0; i < pendingUsers.size(); ++i) {
            cout << i + 1 << ". " << pendingUsers[i] << "\n";
        }

        int choice;
        cout << "Select a user to approve (number): ";
        cin >> choice;
        if (choice > 0 && choice <= pendingUsers.size()) {
            securityManager.approveUser(currentUser, pendingUsers[choice - 1]);
            cout << "User " << pendingUsers[choice - 1] << " approved.\n";
        } else {
            cout << "Invalid choice.\n";
        }
    }

    void assignRole() {
        auto usersWithRoles = securityManager.getAllUsersWithRoles();
        cout << "Users and their roles:\n";
        for (int i = 0; i < usersWithRoles.size(); ++i) {
            cout << i + 1 << ". " << usersWithRoles[i].first << " - " << usersWithRoles[i].second << "\n";
        }

        int userChoice;
        cout << "Select a user to assign role (number): ";
        cin >> userChoice;

        auto roles = securityManager.getAllRoles();
        cout << "Available Roles:\n";
        for (int i = 0; i < roles.size(); ++i) {
            cout << i + 1 << ". " << roles[i] << "\n";
        }

        int roleChoice;
        cout << "Select a role to assign (number): ";
        cin >> roleChoice;

        if (userChoice > 0 && userChoice <= usersWithRoles.size() && roleChoice > 0 && roleChoice <= roles.size()) {
            securityManager.assignRole(currentUser, usersWithRoles[userChoice - 1].first, roles[roleChoice - 1]);
            cout << "Role " << roles[roleChoice - 1] << " assigned to user " << usersWithRoles[userChoice - 1].first << ".\n";
        } else {
            cout << "Invalid choice.\n";
        }
    }

    void addComponent() {
        string componentName;
        cout << "Enter new component name: ";
        cin >> componentName;
        Component newComponent(componentName);
        try {
            pds.addComponent(newComponent);
            cout << "Component added successfully.\n";
        } catch (const DuplicateComponentException& e) {
            cout << "Failed to add component: " << e.what() << endl;
        }
    }

    void removeComponent() {
        string componentName;
        cout << "Enter component name to remove: ";
        cin >> componentName;
        if (pds.removeComponent(componentName)) {
            cout << "Component removed successfully.\n";
        } else {
            cout << "Failed to remove component or component not found.\n";
        }
    }

    void viewAllUsers() {
        auto usersWithRoles = securityManager.getAllUsersWithRoles();
        cout << "Registered Users:\n";
        for (const auto& user : usersWithRoles) {
            cout << "- " << user.first << " (" << user.second << ")\n";
        }
    }

    void viewComponents() {
        vector<string> components = pds.listAllComponents();
        cout << "Registered Components:\n";
        for (const auto& comp : components) {
            cout << "- " << comp << endl;
        }
    }

    void generateReport() {
        pds.generateSystemReport();
    }

public:
    PDS_CLI() : currentUser(""), currentRole("") {}

    void start() {
        int option = 0;
        do {
            if (currentUser.empty()) {
                mainMenu();
                cin >> option;
                switch (option) {
                    case 1: signIn(); break;
                    case 2: signUp(); break;
                    case 3: cout << "Exiting...\n"; break;
                    default: cout << "Invalid option. Please try again.\n";
                }
            } else if (currentRole == "admin") {
                adminMenu();
                cin >> option;
                switch (option) {
                    case 1: approveUser(); break;
                    case 2: assignRole(); break;
                    case 3: viewAllUsers(); break;
                    case 4: addComponent(); break;
                    case 5: removeComponent(); break;
                    case 6: currentUser = ""; currentRole = ""; break;
                    default: cout << "Invalid option. Please try again.\n";
                }
            } else {
                userMenu();
                cin >> option;
                switch (option) {
                    case 1: viewComponents(); break;
                    case 2: generateReport(); break;
                    case 3: currentUser = ""; currentRole = ""; break;
                    default: cout << "Invalid option. Please try again.\n";
                }
            }
        } while (option != 3 || !currentUser.empty());
    }
};

int main() {
    PDS_CLI cli;
    cli.start();
    return 0;
}
