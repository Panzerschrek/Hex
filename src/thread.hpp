#ifndef THREAD_HPP
#define THREAD_HPP
#include <QThread>


//this thread call object method with (void) signature
template <class T>
class h_Thread: public QThread
{

public:
    h_Thread( void (T::*f)(void), T* this_p= NULL, unsigned int in_loop = false )
    {
        this_pointer= this_p;
        func_pointer= f;
        stopped= false;
        loop_call= in_loop;
    }
    void Stop(){stopped=true;}

protected:
    void run()
    {
        if( loop_call )
        {
            while(!stopped)
            {
                ((this_pointer)->*func_pointer)();
            }
        }
        else
            ((this_pointer)->*func_pointer)();
    }

private:
    T* this_pointer;
    void(T::*func_pointer )(void);
    unsigned int loop_call;
    volatile bool stopped;
};


//this template for calling functions
template<>
class h_Thread<void>: public QThread
{

public:
    h_Thread( void (*f)(void),  unsigned int in_loop = false )
    {
        func_pointer= f;
        loop_call= in_loop;
        stopped= false;
    }
    void Stop(){stopped=true;}

protected:
    void run()
    {
        if( loop_call )
        {
            while( !stopped )
                func_pointer();
        }
        else
            func_pointer();
    }

private:

    void (*func_pointer )(void);
    unsigned int loop_call;
    volatile bool stopped;
};

#endif//THREAD_HPP
