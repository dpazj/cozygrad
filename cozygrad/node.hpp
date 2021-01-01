#pragma once

#include "tensor.hpp"
#include "tensor_ops.hpp"

#include <functional>
#include <vector>

class node
{
    public:
        node();
        node(const node& x);
        node(tensor* x);
        node(tensor& x) : node(&x){};
        ~node();

        void forward();
        void backward();

        tensor* data();
        tensor* gradient();
        std::vector<node*> children();
    
        //operators
        node& operator=(const node& other);
        node& operator+(node& other);
        node& operator-(node& other);
        node& operator*(node& other);

        node& relu();
        node& dot(node& other);
        node& pow(double exponent);

    private:
        void add_child(node *);

        tensor* m_data;
        tensor* m_gradient;

        bool m_owns_data = true;
        bool m_owns_gradient = true;

        std::function<void()> m_forward = [](){};
        std::function<void()> m_backward = [](){};

        std::vector<node*> m_children;
        std::vector<node*> m_node_references;
        node* create_node();
};



node::node()
{
    m_data = new tensor();
    m_gradient = new tensor();
}

node::node(const node& x)
{
    *this = x;
}

node::node(tensor* x)
{
    m_owns_data = false;
    m_data = x;
    m_gradient = new tensor();
}

node::~node()
{
    if(m_owns_data && m_data != nullptr)
    {
        delete m_data;
    }

    if(m_owns_gradient && m_gradient != nullptr)
    {
       delete m_gradient;
    }

    for(const auto& x : m_node_references)
    {
        delete x;
    }
}

void node::forward(){m_forward();}
void node::backward(){m_backward();}

tensor* node::data(){return m_data;}
tensor* node::gradient(){return m_gradient;}
std::vector<node*> node::children(){return m_children;}

void node::add_child(node * x){m_children.push_back(x);}

node* node::create_node()
{
    node* out = new node();
    m_node_references.push_back(out);
    return out;
}




//------------OPERATORS----------------

node& node::operator=(const node& other)
{
    m_data = other.m_data;
    m_gradient = other.m_gradient;
    m_owns_data = false;
    m_owns_gradient = false;
    m_children = other.m_children;
    m_backward = other.m_backward;
    m_forward = other.m_forward;
    return *this;
}


node& node::operator+(node& other)
{
    node* out = create_node();

    out->add_child(this);
    out->add_child(&other);

    std::function<void()> forward = [&, out](){ 
        *out->m_data = *m_data + *other.m_data;
    };

    std::function<void()> backward = [&, out](){
        *m_gradient = m_gradient->size() == 0 ? *out->m_gradient : *m_gradient + *out->m_gradient;
        *other.m_gradient = other.m_gradient->size() == 0 ? *out->m_gradient : *other.m_gradient + *out->m_gradient;
    };

    out->m_forward = forward;
    out->m_backward = backward;

    return *out;
}

node& node::operator-(node& other)
{
    node* out = create_node();
    out->add_child(this);
    out->add_child(&other);

    std::function<void()> forward = [&, out](){ 
        *out->m_data = *m_data - *other.m_data;
    };

    std::function<void()> backward = [&, out](){
        *m_gradient = m_gradient->size() == 0 ? *out->m_gradient : *m_gradient + *out->m_gradient;
        *other.m_gradient = other.m_gradient->size() == 0 ? -*out->m_gradient : *other.m_gradient - *out->m_gradient;
    };

    out->m_forward = forward;
    out->m_backward = backward;

    return *out;
}

node& node::operator*(node& other)
{
    node* out = create_node();
    out->add_child(this);
    out->add_child(&other);

    std::function<void()> forward = [&, out](){ 
        *out->m_data = *m_data * *other.m_data;
    };

    std::function<void()> backward = [&, out](){
        auto a_derivative = *out->m_gradient * *other.m_data;
        auto b_derivative = *out->m_gradient * *m_data;

        *m_gradient = m_gradient->size() == 0 ? a_derivative : *m_gradient + a_derivative;
        *other.m_gradient = other.m_gradient->size() == 0 ? b_derivative : *other.m_gradient + b_derivative;
    };

    out->m_forward = forward;
    out->m_backward = backward;

    return *out;
}

node& node::relu()
{
    node* out = create_node();
    out->add_child(this);

    std::function<void()> forward = [&, out](){ 
        *out->m_data = op::max(*m_data, 0.0f);
    };

    std::function<void()> backward = [&, out](){
        auto a_grad = *out->m_gradient * (*m_data > 0.0f);
        *m_gradient = m_gradient->size() == 0 ? a_grad : *m_gradient + a_grad;
    };
    out->m_forward = forward;
    out->m_backward = backward;

    return *out;
}

node& node::dot(node& other)
{
    node* out = create_node();

    out->add_child(this);
    out->add_child(&other);

    std::function<void()> forward = [&, out](){ 
        *out->m_data = op::dot(*m_data,*other.m_data);
    };

    std::function<void()> backward = [&, out](){
        auto a = op::dot(*out->m_gradient, op::transpose(*other.m_data));
        auto b = op::dot(op::transpose(*m_data), *out->m_gradient);

        *m_gradient = m_gradient->size() == 0 ? a: *m_gradient + a;
        *other.m_gradient = other.m_gradient->size() == 0 ? b : *other.m_gradient + b;
    };

    out->m_forward = forward;
    out->m_backward = backward;

    return *out;
}

node& node::pow(double exponent)
{
    node* out = create_node();
    out->add_child(this);

    std::function<void()> forward = [&, out, exponent](){ 
        *out->m_data = op::pow(*m_data, exponent);
    };

    std::function<void()> backward = [&, out, exponent](){
        auto der = *out->m_gradient * op::pow(*m_data, exponent - 1.0f) * exponent;
        *m_gradient = m_gradient->size() == 0 ? der : *m_gradient + der;
    };

    out->m_forward = forward;
    out->m_backward = backward;

    return *out;
}