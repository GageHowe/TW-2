Implementation of client prediction, server reconciliation in UE5 for the ThreadWraith project. This method gives us two things: 1) server authority, which prevents cheating, 2) events like collisions are simulated to happen at the exact same time on every client, regardless of latency, 3) desyncs are almost impossible unless you're connection is unstable or you're dropping a lot of packets

To build this, you'll need Unreal Engine 5.5.3, Visual Studio or JetBrains Rider, and the .Net SDK.
