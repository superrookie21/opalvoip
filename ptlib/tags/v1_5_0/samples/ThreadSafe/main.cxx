/*
 * main.cxx
 *
 * PWLib application source file for ThreadSafe
 *
 * Main program entry point.
 *
 * Copyright 2002 Equivalence
 *
 * $Log: main.cxx,v $
 * Revision 1.3  2002/12/11 03:38:45  robertj
 * Added more tests
 *
 * Revision 1.2  2002/05/02 00:30:26  robertj
 * Added dump of thread times during start up.
 *
 * Revision 1.1  2002/05/01 04:16:44  robertj
 * Added thread safe collection classes.
 *
 */

#include <ptlib.h>
#include "main.h"

#include <ptclib/random.h>


PCREATE_PROCESS(ThreadSafe);


///////////////////////////////////////////////////////////////////////////////

TestObject::TestObject(ThreadSafe & proc, unsigned val)
  : process(proc)
{
  value = val;

  process.mutexObjects.Wait();
  process.totalObjects++;
  process.currentObjects++;
  process.mutexObjects.Signal();
}


TestObject::~TestObject()
{
  process.mutexObjects.Wait();
  process.currentObjects--;
  process.mutexObjects.Signal();
}


PObject::Comparison TestObject::Compare(const PObject & obj)
{
  PAssert(obj.IsDescendant(Class()), PInvalidCast);
  unsigned othervalue = ((const TestObject &)obj).value;
  if (value < othervalue)
    return LessThan;
  if (value > othervalue)
    return GreaterThan;
  return EqualTo;
}


void TestObject::PrintOn(ostream & strm) const
{
  strm << value;
}


///////////////////////////////////////////////////////////////////////////////

ThreadSafe::ThreadSafe()
  : PProcess("Equivalence", "ThreadSafe", 1, 0, AlphaCode, 1)
{
  threadCount = 0;
  totalObjects = 0;
  currentObjects = 0;
}


ThreadSafe::~ThreadSafe()
{
  unsorted.RemoveAll();
  sorted.RemoveAll();
  sparse.RemoveAll();
}


void ThreadSafe::Main()
{
  PArgList & args = GetArguments();
  args.Parse("1-test1."
             "2-test2."
             "3-test3."
             "t-trace.");

  if (args.HasOption('t'))
    PTrace::Initialise(args.GetOptionCount('t'));

  if (args.HasOption('1'))
    Test1(args);
  else if (args.HasOption('2'))
    Test2(args);
  else if (args.HasOption('3'))
    Test3(args);
}


void ThreadSafe::Test1(PArgList & args)
{
  if (args.GetCount() > 0)
    threadCount = args[0].AsUnsigned();
  else
    threadCount = 99;

  cout << "Starting " << threadCount << " threads." << endl;

  for (PINDEX i = 0; i < threadCount; i++) {
    PTimeInterval duration = PRandom::Number()%540000 + 60000;
    cout << setw(4) << (i+1) << '=' << duration;
    if (i%5 == 4)
      cout << '\n';
    PThread::Create(PCREATE_NOTIFIER(Test1Thread), (INT)duration.GetMilliSeconds());
  }
  cout << endl;

  startTick = PTimer::Tick();
  while (threadCount > 0) {
    Test1Output();
    Sleep(5000);
  }

  Test1Output();
  sorted.RemoveAll();
  unsorted.RemoveAll();
  sparse.RemoveAll();
  Test1Output();
}


void ThreadSafe::Test1Output()
{
  sorted.DeleteObjectsToBeRemoved();
  unsorted.DeleteObjectsToBeRemoved();
  sparse.DeleteObjectsToBeRemoved();

  cout << setprecision(0) << setw(5) << (PTimer::Tick()-startTick)
       << " Threads=" << threadCount
       << ", Unsorted=" << unsorted.GetSize()
       << ", Sorted=" << sorted.GetSize()
       << ", Dictionary=" << sparse.GetSize()
       << ", Objects=";

  mutexObjects.Wait();
  cout << currentObjects << '/' << totalObjects;
  mutexObjects.Signal();

  cout << endl;
}


void ThreadSafe::Test1Thread(PThread &, INT duration)
{
  PRandom random;
  PSafePtr<TestObject> ptr;

  PTimer timeout = duration;

  while (timeout.IsRunning()) {
    switch (random%15) {
      case 0 :
        if (random%(unsorted.GetSize()+1) == 0)
          unsorted.Append(new TestObject(*this, random));
        break;

      case 1 :
        if (random%(sorted.GetSize()+1) == 0)
          sorted.Append(new TestObject(*this, random));
        break;

      case 2 :
        sparse.SetAt(random%20, new TestObject(*this, random));
        break;

      case 3 :
        for (ptr = unsorted.GetWithLock(0, PSafeReference); ptr != NULL; ++ptr) {
          if (random%50 == 0)
            unsorted.Remove(ptr);
        }
        break;

      case 4 :
        for (ptr = sorted.GetWithLock(0, PSafeReference); ptr != NULL; ++ptr) {
          if (random%50 == 0)
            sorted.Remove(ptr);
        }
        break;

      case 5 :
        sparse.RemoveAt(random%20);
        break;

      case 6 :
        for (ptr = unsorted; ptr != NULL; ++ptr)
          Sleep(random%50);
        break;

      case 7 :
        for (ptr = sorted; ptr != NULL; ++ptr)
          Sleep(random%50);
        break;

      case 8 :
        for (ptr = sparse; ptr != NULL; ++ptr)
          Sleep(random%50);
        break;

      case 9 :
        for (ptr = unsorted.GetWithLock(0, PSafeReadOnly); ptr != NULL; ++ptr)
          Sleep(random%50);
        break;

      case 10 :
        for (ptr = sorted.GetWithLock(0, PSafeReadOnly); ptr != NULL; ++ptr)
          Sleep(random%50);
        break;

      case 11 :
        for (ptr = sparse.GetWithLock(0, PSafeReadOnly); ptr != NULL; ++ptr)
          Sleep(random%50);
        break;

      case 12 :
        for (ptr = unsorted.GetWithLock(0, PSafeReference); ptr != NULL; ++ptr)
          Sleep(random%50);
        break;

      case 13 :
        for (ptr = sorted.GetWithLock(0, PSafeReference); ptr != NULL; ++ptr)
          Sleep(random%50);
        break;

      case 14 :
        for (ptr = sparse.GetWithLock(0, PSafeReference); ptr != NULL; ++ptr)
          Sleep(random%50);
        break;
    }
    Sleep(random%500);
  }

  threadCount--;
}


void ThreadSafe::Test2(PArgList &)
{
  sparse.SetAt(0, new TestObject(*this, 0));

  threadCount = 2;
  PThread::Create(PCREATE_NOTIFIER(Test2Thread1));
  PThread::Create(PCREATE_NOTIFIER(Test2Thread2));

  while (threadCount > 0)
    Sleep(1000);
}


void ThreadSafe::Test2Thread1(PThread &, INT)
{
  cout << "Thread 1 before read only lock" << endl;
  PSafePtr<TestObject> ptr = sparse.FindWithLock(0, PSafeReadOnly);

  cout << "Thread 1 after read only lock, pausing ..." << endl;
  Sleep(3000);

  cout << "Thread 1 before read write lock" << endl;
  ptr = sparse.FindWithLock(0, PSafeReadWrite);

  cout << "Thread 1 after read write lock, exiting" << endl;

  threadCount--;
}


void ThreadSafe::Test2Thread2(PThread &, INT)
{
  Sleep(1000);

  cout << "Thread 2 before read write lock" << endl;
  PSafePtr<TestObject> ptr = sparse.FindWithLock(0, PSafeReadWrite);

  cout << "Thread 2 after read write lock, exiting" << endl;
  threadCount--;
}


void ThreadSafe::Test3(PArgList &)
{
  for (PINDEX i = 0; i < 10; i++)
    unsorted.Append(new TestObject(*this, i));

  threadCount = 2;
  PThread::Create(PCREATE_NOTIFIER(Test3Thread1));
  PThread::Create(PCREATE_NOTIFIER(Test3Thread2));

  while (threadCount > 0)
    Sleep(1000);
}


void ThreadSafe::Test3Thread1(PThread &, INT)
{
  {
    cout << "Thread 1 before read only lock" << endl;
    PSafePtr<TestObject> ptr = unsorted.GetWithLock(2, PSafeReadOnly);

    cout << "Thread 1 after read only lock, pausing ..." << endl;
    Sleep(2000);

    cout << "Thread 1 before read write lock" << endl;
    ptr.SetSafetyMode(PSafeReadWrite);

    cout << "Thread 1 after read write lock, before ptr going out of scope" << endl;
  }
  cout << "Thread 1 after ptr out of scope, exiting" << endl;

  threadCount--;
}


void ThreadSafe::Test3Thread2(PThread &, INT)
{
  Sleep(1000);

  cout << "Thread 2 before enumeration" << endl;
  PSafePtr<TestObject> ptr = unsorted.GetWithLock(0, PSafeReadOnly);
  while (ptr != NULL) {
    if (ptr->value == 2) {
      cout << "Thread 2 before read write lock" << endl;
      ptr->LockReadWrite();
      cout << "Thread 2 after read write lock" << endl;

      Sleep(2000);

      cout << "Thread 2 before read write unlock" << endl;
      ptr->UnlockReadWrite();
      cout << "Thread 2 after read write unlock" << endl;
    }
    ptr++;
  }

  cout << "Thread 2 after enumeration, exiting" << endl;
  threadCount--;
}


// End of File ///////////////////////////////////////////////////////////////
