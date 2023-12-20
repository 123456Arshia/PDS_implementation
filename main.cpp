//
//  main.cpp
//  pds
//
//  Created by Arshia Taghavinejad on 2023-12-20.
//

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
using namespace std;

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

    Service(const string& serviceName, int version) : serviceName(serviceName), version(version) {}
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
    

};
int main() {
    // Create the PDS instance
    PDS pds;

    // Create services
    auto service1 = make_shared<Service>("DatabaseService", 1);
    auto service2 = make_shared<Service>("LoggingService", 1);
    auto service3 = make_shared<Service>("AuthenticationService", 1);

    // Create components
    Component comp1("WebServer");
    Component comp2("ApplicationServer");

    // Add services to components
    comp1.addExport(service1); // WebServer exports DatabaseService
    comp1.addImport(service2); // WebServer imports LoggingService
    comp2.addExport(service2); // ApplicationServer exports LoggingService
    comp2.addImport(service1); // ApplicationServer imports DatabaseService
    comp2.addImport(service3); // ApplicationServer imports AuthenticationService

    // Add components to PDS
    pds.addComponent(comp1);
    pds.addComponent(comp2);

    // List all components
    cout << "List of all components:\n";
    vector<string> componentNames = pds.listAllComponents();
    for (const string& componentName : componentNames) {
        cout << componentName << "\n";
    }

    // Get component by name
    cout << "\nGet 'WebServer' component details:\n";
    pds.displayComponentDetails("WebServer");

    // Update a component
    cout << "\nUpdating 'WebServer' component name to 'UpdatedWebServer':\n";
    Component updatedComp("UpdatedWebServer");
    if (pds.updateComponent("WebServer", updatedComp)) {
        cout << "Component updated successfully.\n";
    } else {
        cout << "Component update failed.\n";
    }

    // Remove a component
    cout << "\nRemoving 'ApplicationServer' component:\n";
    if (pds.removeComponent("ApplicationServer")) {
        cout << "Component removed successfully.\n";
    } else {
        cout << "Component removal failed.\n";
    }

    // Check for circular dependencies
    cout << "\nChecking for circular dependencies:\n";
    if (pds.hasCircularDependencies()) {
        cout << "Circular dependencies detected.\n";
    } else {
        cout << "No circular dependencies found.\n";
    }

    // Update service version
    cout << "\nUpdating 'DatabaseService' to version 2:\n";
    pds.updateServiceVersion("DatabaseService", 2);

    // Perform a system health check
    cout << "\nPerforming System Health Check:\n";
    if (pds.performSystemHealthCheck()) {
        cout << "System is healthy.\n";
    } else {
        cout << "System has unresolved issues.\n";
    }

    // Simulate a system change
    cout << "\nSimulating a change in 'LoggingService':\n";
    pds.simulateSystemChange("LoggingService");

    // Rollback component update
    cout << "\nRolling back 'UpdatedWebServer' component update:\n";
    if (pds.rollbackComponentUpdate("UpdatedWebServer")) {
        cout << "Component update rolled back successfully.\n";
    } else {
        cout << "Component update rollback failed.\n";
    }

    // Find components by service
    cout << "\nFinding components that use 'DatabaseService':\n";
    vector<string> componentsUsingService = pds.findComponentsByService("DatabaseService");
    for (const string& componentName : componentsUsingService) {
        cout << componentName << " uses 'DatabaseService'.\n";
    }

    // Deactivate service
    cout << "\nDeactivating 'DatabaseService':\n";
    pds.deactivateService("DatabaseService");

    // List imports by component
    cout << "\nList imports for 'UpdatedWebServer' component:\n";
    vector<string> importsList = pds.listImportsByComponent("UpdatedWebServer");
    if (importsList.empty()) {
        cout << "'UpdatedWebServer' has no imports.\n";
    } else {
        cout << "'UpdatedWebServer' imports the following services:\n";
        for (const string& import : importsList) {
            cout << import << "\n";
        }
    }

    // Validate exports have imports
    cout << "\nValidating exports have imports:\n";
    if (pds.validateExportsHaveImports()) {
        cout << "All exports have corresponding imports.\n";
    } else {
        cout << "Some exports do not have corresponding imports.\n";
    }

    // Notify service updates
    cout << "\nNotifying updates for 'DatabaseService' version 3:\n";
    pds.notifyServiceUpdates("DatabaseService", 3);

    // Display services with version
    cout << "\nDisplaying services with their versions:\n";
    pds.displayServicesWithVersion();

    // Balance service load
    cout << "\nBalancing service load:\n";
    pds.balanceServiceLoad();

    // Automate service updates
    cout << "\nAutomating service updates:\n";
    pds.automateServiceUpdates();

    // Generate system report
    cout << "\nGenerating System Report:\n";
    pds.generateSystemReport();

    return 0;
}
