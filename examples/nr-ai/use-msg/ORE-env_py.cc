#include "ORE-env.h"

#include <ns3/ai-module.h>

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(ns3ai_ore_py_stru, m)
{
    py::class_<ns3::OREEnvStruct>(m, "PyEnvStruct")
        .def(py::init<>())
        .def_readwrite("imsi", &ns3::OREEnvStruct::imsi)
        .def_readwrite("time", &ns3::OREEnvStruct::time)
        .def_readwrite("occupiedResources", &ns3::OREEnvStruct::occupiedResources)
        .def_readwrite("rsrpThreshold", &ns3::OREEnvStruct::rsrpThreshold)
        .def_readwrite("srcRnti", &ns3::OREEnvStruct::srcRnti)
        .def_readwrite("Rc", &ns3::OREEnvStruct::Rc)
        .def_readwrite("srcSlotNum", &ns3::OREEnvStruct::srcSlotNum)
        .def_readwrite("srcScNum", &ns3::OREEnvStruct::srcScNum)
        .def_readwrite("index", &ns3::OREEnvStruct::index)
        .def_readwrite("estimatorUpdateFlag", &ns3::OREEnvStruct::estimatorUpdateFlag)
        .def_readwrite("addRecievedPacketFlag", &ns3::OREEnvStruct::addRecievedPacketFlag)
        .def_readwrite("selectionModeSelectionFlag", &ns3::OREEnvStruct::selectionModeSelectionFlag)
        .def_readwrite("getNumResourcesSelectedFlag", &ns3::OREEnvStruct::getNumResourcesSelectedFlag)
        .def_readwrite("getChosenSlotFlag", &ns3::OREEnvStruct::getChosenSlotFlag)
        .def_readwrite("getChosenSubChannelFlag", &ns3::OREEnvStruct::getChosenSubChannelFlag)
        .def_readwrite("NSeQueryFlag", &ns3::OREEnvStruct::NSeQueryFlag)
        .def_readwrite("firstCallFlag", &ns3::OREEnvStruct::firstCallFlag)
        .def_readwrite("encodedSrcRnti", &ns3::OREEnvStruct::encodedSrcRnti)
        .def_readwrite("encodedRc", &ns3::OREEnvStruct::encodedRc)
        .def_readwrite("encodedSrcSlot", &ns3::OREEnvStruct::encodedSrcSlot)
        .def_readwrite("encodedSrcSc", &ns3::OREEnvStruct::encodedSrcSc)
        .def_readwrite("sensingDataFlag", &ns3::OREEnvStruct::sensingDataFlag);

    py::class_<ns3::OREActStruct>(m, "PyActStruct")
        .def(py::init<>())
        .def_readwrite("selectionMode", &ns3::OREActStruct::selectionMode)
        .def_readwrite("NSe", &ns3::OREActStruct::NSe)
        .def_readwrite("numResourcesSelected", &ns3::OREActStruct::numResourcesSelected)
        .def_readwrite("chosenSlot", &ns3::OREActStruct::chosenSlot)
        .def_readwrite("chosenSubChannel", &ns3::OREActStruct::chosenSubChannel)
        .def_readwrite("encodedSelectionInstructions", &ns3::OREActStruct::encodedSelectionInstructions);

    py::class_<
        ns3::Ns3AiMsgInterfaceImpl<ns3::OREEnvStruct, ns3::OREActStruct>>(
        m,
        "Ns3AiMsgInterfaceImpl")
        .def(py::init<bool,
                      bool,
                      bool,
                      uint32_t,
                      const char*,
                      const char*,
                      const char*,
                      const char*>())
        .def("PyRecvBegin",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::OREEnvStruct,
                                         ns3::OREActStruct>::PyRecvBegin)
        .def("PyRecvEnd",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::OREEnvStruct,
                                         ns3::OREActStruct>::PyRecvEnd)
        .def("PySendBegin",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::OREEnvStruct,
                                         ns3::OREActStruct>::PySendBegin)
        .def("PySendEnd",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::OREEnvStruct,
                                         ns3::OREActStruct>::PySendEnd)
        .def("PyGetFinished",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::OREEnvStruct,
                                         ns3::OREActStruct>::PyGetFinished)
        .def("GetCpp2PyStruct",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::OREEnvStruct,
                                         ns3::OREActStruct>::GetCpp2PyStruct,
             py::return_value_policy::reference)
        .def("GetPy2CppStruct",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::OREEnvStruct,
                                         ns3::OREActStruct>::GetPy2CppStruct,
             py::return_value_policy::reference);
}