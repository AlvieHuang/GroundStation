#include "uav.h"
#include <QDateTime>
#include <math.h>

typedef unsigned char u_int8_t;

int UAV::NUM_RECV_PACKETS = 0;
UAV::UAV(QWidget *parent)
//    : QDialog(parent)
{
    // Set booleans to simulate UAV state
    uavOn = false;
    receivedInfoPacketReq = false;
    uavWaypointsReady = false;
    uavFlying = false;
    uavFlyingHome = false;
    stopAction = false;
    shutdownAction = false;

    // Set constants for packets
    battery = 100;
    pointsStorable = 20;
    telemSeqNumber = 1;
    currentNumOfPoints = 0;
    uavLat = 33.6454;
    uavLng = -117.8426;
    uavHomeLat = uavLat;
    uavHomeLng = uavLng;

    uavMass = 2.0;
    uavXAccel = 0.0008;
    uavYAccel = 0.0008;
    uavZAccel = 0.1;

    uavPitch = 0;
    uavRoll = 0;
    uavYaw = 0;
    uavXVel = 0;
    uavYVel = 0;
    uavZVel = 0;
    uavAlt = 1;

    latLngSpd = .002;

    // Set up qtimer for telemetry packets every 200 ms
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(sendCurrentTelem()));

    // Connect receiving socket to the specified received port
    recvUdpSocket.bind(UAV::UAV_PORT_NUM);
//    recvUdpSocket.bind(20175);

    // Connect receive socket to call the method to process pending datagrams
    connect(&recvUdpSocket, SIGNAL(readyRead()),
                this, SLOT(processPendingDatagrams()));

    QTextStream(stdout) << "Listening for packets.." << endl;
}

void UAV::sendAllActionPackets(std::vector<Protocol::Packet*> packets)
{
    //QTextStream(stdout) << "The size of the vector is " << packets.size() << endl;
    for(auto i = packets.begin(); i != packets.end(); ++i)
    {
        sendAPacket(*i);
    }

}


void UAV::sendAllActionPackets(std::queue<Protocol::Packet*> packets)
{
    //QTextStream(stdout) << "The size of the vector is " << packets.size() << endl;
    int size = packets.size();
    for(int i = 0; i < size; ++i)
    {
        sendAPacket(packets.front());
        packets.pop();
    }

}

void UAV::sendAPacket(Protocol::Packet* packet)
{
    QByteArray datagram;
    QDataStream out(&datagram, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_3);
    
    // Allocate storage for the packet in the for of u_int8_t
    u_int8_t storage[UAV::PACKET_LENGTH];
    
    // Convert the packet into bytes and store into storage
    size_t packet_size = packet->GetBytes(storage, UAV::PACKET_LENGTH);

    // Send bytes inside storage to out datastream
    for(size_t i =0; i < packet_size; i++)
    {
        out << storage[i];
    }

    // Send datagram through UDP socket
    sendUdpSocket.writeDatagram(datagram, QHostAddress::LocalHost, UAV::GS_PORT_NUM);
//    sendUdpSocket.writeDatagram(datagram, QHostAddress('10.0.2.15'), UAV::GS_PORT_NUM);
}



void UAV::processPendingDatagrams()
{
    QTextStream(stdout) << "data recieved!" << endl;
    while(recvUdpSocket.hasPendingDatagrams())
    {
        QTextStream(stdout) << "Processing started" << endl;
        QByteArray datagram;
        datagram.resize(recvUdpSocket.pendingDatagramSize());

        // Read from the udpSocket while there is a datagram and store into datagram.
        recvUdpSocket.readDatagram(datagram.data(), datagram.size());

        // Validates check sum first and then convert Datagram into proper packet.
        Protocol::Packet* packet = Protocol::Packet::Parse((uint8_t*)datagram.data(), datagram.size());
        Protocol::PacketType packet_type = packet->get_type();

            

        // Depending on the type call the proper method to extract packet's information and print
        if(packet != nullptr)
        {
            QTextStream(stdout) << "Packet number " << UAV::NUM_RECV_PACKETS + 1 << endl;
            switch(packet_type)
            {
                case Protocol::PacketType::Action:
                    print_action_packet(*dynamic_cast<Protocol::ActionPacket*>(packet));
                    respond_to_action_packet(*dynamic_cast<Protocol::ActionPacket*>(packet));
                    sendAckPacket(packet, "Action Packet");
                    break;
                case Protocol::PacketType::Ack:
                    print_ack_packet(*dynamic_cast<Protocol::AckPacket*>(packet));
                    break;
                case Protocol::PacketType::Info:
                    print_info_packet(*dynamic_cast<Protocol::InfoPacket*>(packet));
                    sendAckPacket(packet, "Info Packet");
                    break;
                case Protocol::PacketType::Telem:
                    print_telemetry_packet(*dynamic_cast<Protocol::TelemetryPacket*>(packet));
                    sendAckPacket(packet, "Telemetry Packet");
                    break;
                default:
                    break;
            }

            QTextStream(stdout) << ""<< endl;
            ++UAV::NUM_RECV_PACKETS;
        }
        else
        {
            QTextStream(stdout) << "ERROR: Packet is invalid" << endl;
        }
    }
}

void UAV::print_telemetry_packet(Protocol::TelemetryPacket& packet)
{
    float   vx, vy, vz,
            pitch, roll, yaw,
            heading, alt;

    double  lat, lon;

    // Extract all information from telemetry packet into variables
    packet.GetVelocity(&vx, &vy, &vz);
    packet.GetOrientation(&pitch, &roll, &yaw);
    packet.GetLocation(&lat, &lon, &alt);
    packet.GetHeading(&heading);

    // Print out information
    QTextStream(stdout) << "Type: Telemetry Packet" << endl;
    QTextStream(stdout) << "Velocity x: "   << vx << endl
                        << "Velocity y: "   << vy << endl
                        << "Velocity z: "   << vz << endl;
    QTextStream(stdout) << "Pitch: "        << pitch << endl
                        << "Roll: "         << roll << endl
                        << "Yaw: "          << yaw << endl;
    QTextStream(stdout) << "Latitude: "     << lat << endl
                        << "Longitude: "    << lon << endl
                        << "Altitude: "     << alt << endl;
    QTextStream(stdout) << "Heading: "      << heading << endl;

}

//    QTextStream(stdout) << altitude;

void UAV::print_ack_packet(Protocol::AckPacket& packet){
    QTextStream(stdout) << "Type: AckPacket" << endl;

}
void UAV::print_action_packet(Protocol::ActionPacket& packet){
    double 	lat, lon;
    float	alt, speed;
 
    QTextStream(stdout) << "Type: ActionPacket" << endl;
    Protocol::ActionType type = packet.GetAction();

    switch(type)
    {
        case Protocol::ActionType::Start : QTextStream(stdout) << "Start: " << (uint8_t)type << endl; break;
        case Protocol::ActionType::RequestInfo : QTextStream(stdout) << "Request Info: " << (uint8_t)type << endl; break;
        case Protocol::ActionType::AddWaypoint : QTextStream(stdout) << "Add Waypoint: " << (uint8_t)type << endl; break;
        case Protocol::ActionType::SetHome : QTextStream(stdout) << "Set Home: " << (uint8_t)type << endl; break;
        case Protocol::ActionType::RemoveWaypoint : QTextStream(stdout) << "Remove Waypoint: " << (uint8_t)type << endl; break;
        case Protocol::ActionType::Stop : QTextStream(stdout) << "Stop: " << (uint8_t)type << endl; break;
        case Protocol::ActionType::Shutdown : QTextStream(stdout) << "Shutdown: " << (uint8_t)type << endl; break;
        default :   QTextStream(stdout) << "Unknown Type: " << (uint8_t)type << endl; break;
    }

    Protocol::Waypoint waypoint = packet.GetWaypoint();
    lat = waypoint.lat;
    lon = waypoint.lon;
    alt = waypoint.alt;
    speed = waypoint.speed;

    QTextStream(stdout) << "Latitude: " << lat << endl;
    QTextStream(stdout) << "Longitude: " << lon << endl;
    QTextStream(stdout) << "Altitude: " << alt << endl;
    QTextStream(stdout) << "Speed: " << speed << endl;

}

void UAV::print_info_packet(Protocol::InfoPacket &packet){
    QTextStream(stdout) << "Type: InfoPacket" << endl;
    QTextStream(stdout) << "Points Storable: " << packet.GetStorable() << endl;
    QTextStream(stdout) << "Battery State: " << packet.GetBattery() << endl;
    QTextStream(stdout) << "Other : " << QString::fromStdString(packet.GetOther()) << endl;
 }


void UAV::respond_to_action_packet(Protocol::ActionPacket ap)
{
    Protocol::ActionType ap_type = ap.GetAction();

    // Simple one packet send first.
    switch(ap_type)
    {
        case Protocol::ActionType::RequestInfo:
            if(uavOn)
                send_info_packet();
            
            break;
        case Protocol::ActionType::AddWaypoint:
            if(uavOn && can_add_waypoint())
            {
                QTextStream(stdout) << "Adding new waypoint" << endl;
                uavWaypointsReady = true;
                ++currentNumOfPoints;
                addWaypoint(ap);
            }
            else
                QTextStream(stdout) << "Cannot add waypoint." << endl;
            break;
        case Protocol::ActionType::SetHome:
            {
            Protocol::Waypoint newHome = ap.GetWaypoint();
            QTextStream(stdout) << "Setting Home: (" << newHome.lat << "," << newHome.lon << ")" << endl;
            uavHomeLat = newHome.lat;
            uavHomeLng = newHome.lon;
            }
            break;
        case Protocol::ActionType::Stop:
            QTextStream(stdout) << "STOPPING" << endl;
            uavFlying = false;
            stopAction = true;
            exit(0);
            break;
        case Protocol::ActionType::Start:
            //if(uavOn && !receivedInfoPacketReq && uavWaypointsReady)
            if(!uavOn)
            {
                QTextStream(stdout) << "STARTING... "<< endl;
                uavOn = true;
                uavFlying = true;
                receivedInfoPacketReq = true;
                timer->start(200);                
            }
            break;
        case Protocol::ActionType::Shutdown:
            shutdownAction = true;
            uavOn = false;
            timer->stop();
            break;
        default:
            break;

    }
}

void UAV::send_info_packet()
{
    Protocol::InfoPacket ip;
    ip.SetBattery(battery);
    ip.SetStorable(pointsStorable);

    sendAPacket(dynamic_cast<Protocol::Packet*>(&ip));
}

void UAV::sendCurrentTelem()
{
    updateUavPosition();
    Protocol::TelemetryPacket tp(telemSeqNumber++);
    tp.SetVelocity(uavXVel,uavYVel,uavZVel);
    tp.SetOrientation(uavPitch,uavRoll,uavYaw);
    tp.SetLocation(uavLat, uavLng, uavAlt);
    tp.SetHeading(uavHeading);
    sendAPacket(&tp);
}

bool UAV::can_add_waypoint()
{
    return currentNumOfPoints + 1 < pointsStorable;
}

void UAV::addWaypoint(Protocol::ActionPacket ap)
{
    //QTextStream(stdout) << "Waypoint added" << endl;
    Protocol::Waypoint wp = ap.GetWaypoint();
    
    pointOfInterest.push(wp);
}

void UAV::updateUavPosition() {

    if(pointOfInterest.empty()){
        return;
    }
    Protocol::Waypoint nextPoint = pointOfInterest.front();
    //latLngSpd = nextPoint.speed;

    double deltaLng = nextPoint.lon - uavLng;
    double deltaLat = nextPoint.lat - uavLat;
    double deltaAlt = nextPoint.alt - uavAlt; //unused
    double distance = sqrt(deltaLng*deltaLng+deltaLat*deltaLat);

    double speed = latLngSpd;
    double altSpeed = 0.2;

    double dLng = deltaLng/distance*speed;
    double dLat = deltaLat/distance*speed;
    double dAlt = deltaAlt/fabs(deltaAlt)*altSpeed;

    uavHeading = atan2(dLat,dLng);

    /// \todo change these to be relative to the CG of the UAV

    double dampingC = 0.026*(1000.0/uavAlt)*10000;
    double damping = 1/(dampingC*speed*speed+1);

    uavXVel += uavXAccel*dLat/speed/uavMass;
    uavYVel += uavYAccel*dLng/speed/uavMass;
    uavZVel += uavZAccel*dAlt/altSpeed/uavMass;

    uavXVel *= damping;
    uavYVel *= damping;
    uavZVel *= damping*damping;

    uavLat += uavXVel;
    uavLng += uavYVel;
    uavAlt += uavZVel;

    uavPitch = 0;
    uavRoll = 0;
    uavYaw = uavHeading;

    battery -= 0.1;
    if(battery < 20.0){
        send_info_packet();
    }

    if(!uavFlyingHome && distance < speed && pointOfInterest.size() > 0)
    {
        Protocol::ActionPacket waypointPacket;
        waypointPacket.SetAction(Protocol::ActionType::AddWaypoint);
        waypointPacket.SetWaypoint(nextPoint);
        sendAPacket(&waypointPacket);
        QTextStream(stdout) << "Destination reached: (" << nextPoint.lat << ", " << nextPoint.lon << "). Sent waypoint packet" << endl;
        pointOfInterest.pop();
    }
    else if(!uavFlyingHome && pointOfInterest.size() == 0)
    {
        QTextStream(stdout) << "Nowhere else to go... Heading Home." << endl;
        uavFlyingHome = true;
        Protocol::Waypoint homePoint;
        homePoint.lat = uavHomeLat;
        homePoint.lon = uavHomeLng;
        pointOfInterest.push(homePoint);
    }
    else if(uavFlyingHome && distance < 2*speed && pointOfInterest.size() > 0)
    {
        Protocol::ActionPacket waypointPacket;
        waypointPacket.SetAction(Protocol::ActionType::AddWaypoint);
        waypointPacket.SetWaypoint(nextPoint);
        sendAPacket(&waypointPacket);
        QTextStream(stdout) << "Home reached. Sent waypoint packet" << endl;
        pointOfInterest.pop();
    }
    else if(uavFlyingHome && pointOfInterest.size() == 0)
    {
        timer->stop();
        uavFlying = false;
        uavWaypointsReady = false;
    }
}


void UAV::sendAckPacket(Protocol::Packet* p, const QString& pt)
{
    Protocol::AckPacket ack;

    ack.set_timestamp(p->get_timestamp());
     
    QTextStream(stdout) << "Sending ack packet " << pt << " and time: " << p->get_timestamp() << endl;

    sendAPacket(&ack);
}
