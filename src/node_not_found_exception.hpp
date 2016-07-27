/*
 * node_not_found_exception.hpp
 *
 *  Created on: 25.07.2016
 *      Author: michael
 */

#ifndef NODE_NOT_FOUND_EXCEPTION_HPP_
#define NODE_NOT_FOUND_EXCEPTION_HPP_


class NodeNotFoundException : public std::runtime_error {
    NodeNotFoundException(const std::string = "Node not found.") : message(m) {}
    ~NodeNotFoundException(void);
    virtual const char* what() const {
        return message.c_str();
    }

private:
    std::string message;
};



#endif /* NODE_NOT_FOUND_EXCEPTION_HPP_ */
