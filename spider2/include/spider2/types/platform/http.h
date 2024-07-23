//
// Created by jhrub on 08.10.2022.
//

#pragma once

#include <boost/beast.hpp>


namespace spider2 {

    /**
     *  Pulling in the beast library
     **/
    using namespace boost::beast;
    using verb = http::verb;
    using status = http::status;
}