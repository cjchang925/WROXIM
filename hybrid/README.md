Noxim - the NoC Simulator
=========================

Welcome to Noxim, the Network-on-Chip Simulator developed at the University of Catania (Italy).
The Noxim simulator is developed using SystemC, a system description language based on C++, and
it can be downloaded from SourceForge under GPL license terms.

If you use Noxim in your research, the authors would appreciate the following citation in any 
publications to which it has contributed:

V. Catania, A. Mineo, S. Monteleone, M. Palesi, D. Patti. Noxim: An Open, Extensible and Cycle-accurate Network on Chip Simulator. *IEEE International Conference on Application-specific Systems, Architectures and Processors 2015. July 27-29, 2015, Toronto, Canada*.

for further information about Noxim, please refer to the file NOXIM_README.md

Noxim with Task Mapping Support
===============================

A set of extensions have been carried out to Noxim simulator to ease the evaluation of different task
mapping mechanisms employed to perform the arrangement of tasks onto the resources of WNoC platforms.

**[October 2016]** Task Mapping Support added to Noxim. Characteristics:

  * Task Mapping Support for the evaluation of Task Mapping Mechanisms.
  * Applications are described through Annotated Task Graphs (ATGs).
  * ATGs and the mapping of tasks are passed to the simulator through regular text files.
  * Simple power estimation model included for processing elements (PEs).
  * Simple Round Robin Scheduler included to support several task execution on the same PE.
  * Optional accurate logs for deep debugging and information (see section added by LGGM at Makefile)

Project
-------

The extensions to Noxim are part of the PhD thesis project called "A Task Mapping Approach with
Reliability Considerations for Multicore Systems based on Wireless Network-on-Chip" being
developed by the student Luis Germán García M. at the Embedded Systems and Computational
Intelligence SISTEMIC research group. It is affiliated to the Electronics Engineering Department,
Faculty of Engineering, University of Antioquia, Colombia. This project is supported by the
Department of Science, Technology and Innovation COLCIENCIAS, Colombia, under the grant 647
of 2014 named Doctorados Nacionales 2014 (Scholarship 2015-2018).

Special Thanks
--------------
Vincenzo Catania, Andrea Mineo, Salvatore Monteleone, Maurizio Palesi and Davide Patti for developing and making available Noxim Simulator.

Quick Start
-----------

If you are working on Ubuntu, you can install noxim and all the dependencies with the following command:

    bash <(wget -qO- --no-check-certificate https://raw.githubusercontent.com/lgermangm/noxim-with-taskmapping/master/other/setup/ubuntu.sh)

Or, to get just the latest master sources, you can run:

    git clone https://github.com/lgermangm/noxim-with-taskmapping.git

For further information about Noxim please refer to the following files contained in the "doc" directory:

  * INSTALL.txt: instructions to install the tool
  * MANUAL.txt: explanation of the arguments that can be passed to the simulator
  * AUTHORS.txt: authors of the tool
  * LICENSE.txt: license under which you are allowed to use the source files

For further information about Noxim with Task Mapping Support please refer to the following files
contained in the "tm_notes" directory:

  * TM_TECHNICALREPORT.pdf: instructions to use Noxim with Task Mapping Support
  * TM_AUTHORS.txt: authors of the extensions

Examples of task mapping simulation files are available in folder "tm_examples".
Logs from the examples previously executed can be found in folder "tm_log".
Scripts and miscellany can be found in folder "tm_scripts".

Example:
  1) Go to noxim/bin folder.
  2) Make sure Noxim has been already compiled.
  3) Execute: ./noxim -config ../tm_examples/01-16-nownoc.yaml -taskmapping ../tm_examples/04-manyapps-manytasksperpe1.map -taskmappinglog ../tm_log/04-manyapps-manytasksperpe1.log
  4) See the results file: ../tm_log/04-manyapps-manytasksperpe1.log


