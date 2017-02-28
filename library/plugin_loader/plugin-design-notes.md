# Design notes for plugin architecture

###Discussion

Currently there are several polymorphic class hierarchies in VidTK where some of the concrete classes are included or excluded based on configuration options or the availability of support libraries. The control of instantiating these optional classes is controlled by #ifdef's in the source code. This solution does not scale well. It creates a lot of coupling between source modules and also creates undesirable transitive dependencies.

In addition, the current method of customizing a build makes it difficult to remove a dependency on an arbitrary external library that does not have explicit support in the CMake files. This causes problems when integrating our software into a customers specific operational environment.

One solution to this problem is to move to a more dynamic approach where functional modules can be loaded at run time. Only those modules that are discovered are available for use. This simplifies creating custom configurations based on external library availability, project needs or customer environment restrictions.

Each plugin can be individually enabled if the required support libraries are found and it is enabled in the CMake script.

The plugin loader should be a general purpose (or as general as possible) mechanism for dynamically creating objects at run time from a set of factories determined at run time.

The plugin loader should be implemented so that it can be easily ported to other projects, such as Kwiver.

## Candidates

VidTK contains several areas where there is a polymorphic class hierarchy where some elements depend on how the product was built or what is available on the system. These are listed below:

- **Track readers and writers** - We support many formats with some being optional based on configuration  selections
- **Image object readers and writers** - There are several formats for these files with some being optional
- **Logger back ends** - Currently this is handled with a separate mechanism because it is done so early during startup and the plugin loader needs to use the logger.
- **Descriptors** - There are many descriptor modules where some are optional and require specific support libraries.
- **Activity detectors** -
- **Video input formats** -
- **Shot stitching algorithms** - The MapTK based shot stitching is optional and could be loaded at run time.
- **Object detectors** -

Even though the plugin manager is conceived as a solution for polymorphic classes, it can also support creation of any single class, such as processes. A super-process could look for a specific process plugin and adapt its pipeline based on if it is available or not. Currently this is done based on #defines or config parameter values.

### Additional requirements

Considering the support required for implementing a plugin system for the above listed candidates, there are some details that would make it easier. For example, file writers need to specify the file type that is written. A name and detailed plugin description would be nice so that a pool of plugins can be inspected to see what functions are available. You could also imagine that a version number would be useful along with some information on how this class interacts with a config block.

Given the set of required plugin metadata and the function specific metadata, a general purpose, extensible approach to adding metadata to plugins is needed.


## Implementation

### Plugin Manager

The plugin manager is implemented as a singleton which discovers all available plugins when it is constructed. A plugin (shared library or DLL) must have exposed a C function `register_items()` in order to be considered a suitable plugin. Other loadable modules can share the same directories if they do not expose this function.

The `register_items()` function is called by the plugin manager to obtain a list of object factories in the shared object. The plugin manager makes a map of all registered object factories.

Since we are mostly supporting polymorphic class hierarchies, each factory creates a concrete object that provides a common interface. The factories are grouped by the plugin manager into lists of factories that provide objects that expose the same interface type. The interface type is used as the key to the map.

For example, for track readers, the interface type would be `track_reader` and the object factory would create a `track_reader_kw18` object.

Applications like data readers would get a list of factories that provide the desired interface to read the data. Each factory in the list can create an object that can be used by the reader to handle a different data file format. An object for each reader type would be collected by the reader process and a config description can be built that lists all currently available reader formats.

### Object Factory

The object factory implements methods to access the attributes and a method to create an object of a specific type. The list of attributes is extensible to handle different types of class hierarchies. For example, a data writer may need to specify the data type it writes. Configuration related help can be added so it can be collected from the set of factories and made available as a parameter description in the config block.

# Still to do

- Rename the actual shared object files to be modules rather than plugins. (??)
- readers/writers should log (debug or trace level) which plugins are used and which ones are rejected.
- Think about attribute data that could be a string or the results of a method call. The data would be an abstract interface and the `std::string const& get_data()` method would be implemented in the concrete class. It would return a value. Typically there would be a constant string, but it could be so much more. This expansion could be deferred by overloading the `add_attribute(std::string const&, std::string const&)` method to be `add_attribute(std::string const&, ActiveAttribute*)`. The interface does not need to change, only the internals need to change.
