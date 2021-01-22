cmake_minimum_required(VERSION 3.9)
project(tfdirDSPonPC)

set(CMAKE_CXX_STANDARD 11)

add_executable(
        tfdirDSPonPC
        main.cpp
        TFDIRContainer.cpp
        TFDIRContainer.h
        Comtrade.cpp
        Comtrade.h
        phasor.cpp
        phasor.h
        data_stream.h
        Constants.h)