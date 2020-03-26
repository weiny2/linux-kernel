========================================================================================
Copyright (C) Intel Corporation, 2013 - 2019
Intel(R) Dynamic Application Loader (Intel(R) DAL) SDK: KDI (Kernel Intel DAL Interface)
========================================================================================

Intel DAL SDK has created this KDI Sample for you as a starting point.

The sample is a simple Linux host application that uses KDI, a DAL Linux kernel module.

The sample loads a trusted application into the Intel® DAL virtual machine (VM), using
the Intel(R) DAL Host Interface Service (JHI), and uses KDI to communicate with the trusted application.

Note: KDI is usually used by Linux modules in kernel space. This sample is run over user space to simplify its operation.
      Typical use case is to create a kernel module to communicate with KDI. See the test module code as an example.

The sample uses the DAL Linux test module, which is compiled under the kernel config CONFIG_INTEL_MEI_DAL_TEST.

Sample Flow
===========
1. Install a trusted application. (See the java code in Bist.java. The application uses the compiled and signed trusted application.)
   The installation is performed from userspace, using teemanagement APIs.
2. Use KDI to open a session to the trusted application. The command is sent to KDI via the Intel DAL test module.
3. Use KDI via the Intel DAL test module to send a command and receive data from the trusted application.
4. Use KDI to close the session.
5. Use the Intel(R) DAL Host Interface Service (JHI) to uninstall the trusted application (optional).

Running the Sample
==================
1. Install CMake
2. Run the following inside the sample directory:
                cmake .
                make
   This will create an executable file: kdi_sample
3. Run the executable:
   ./kdi_sample
