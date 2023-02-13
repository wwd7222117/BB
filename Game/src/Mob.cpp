// MIT License
// 
// Copyright(c) 2020 Arthur Bacon and Kevin Dill
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "Mob.h"

#include "Constants.h"
#include "Game.h"
#include "Player.h"

#include <algorithm>
#include <vector>
#include <math.h>


Mob::Mob(const iEntityStats& stats, const Vec2& pos, bool isNorth)
    : Entity(stats, pos, isNorth)
    , m_pWaypoint(NULL)
{
    assert(dynamic_cast<const iEntityStats_Mob*>(&stats) != NULL);
}

void Mob::tick(float deltaTSec)
{
    // Tick the entity first.  This will pick our target, and attack it if it's in range.
    Entity::tick(deltaTSec);

    // if our target isn't in range, move towards it.
    if (!targetInRange())
    {
        move(deltaTSec);
    }
}

void Mob::move(float deltaTSec)
{
    // If we have a target and it's on the same side of the river, we move towards it.
    //  Otherwise, we move toward the bridge.
    bool bMoveToTarget = false;
    if (!!m_pTarget)
    {
        bool imTop = m_Pos.y < (GAME_GRID_HEIGHT / 2);
        bool otherTop = m_pTarget->getPosition().y < (GAME_GRID_HEIGHT / 2);

        if (imTop == otherTop)
        {
            bMoveToTarget = true;
        }
    }

    Vec2 destPos;
    if (bMoveToTarget)
    {
        m_pWaypoint = NULL;
        destPos = m_pTarget->getPosition();
    }
    else
    {
        if (!m_pWaypoint)
        {
            m_pWaypoint = pickWaypoint();
        }
        destPos = m_pWaypoint ? *m_pWaypoint : m_Pos;
    }

    // Actually do the moving
    Vec2 moveVec = destPos - m_Pos;
    //float distRemaining = moveVec.normalize();
    //float moveDist = m_Stats.getSpeed() * deltaTSec;
    float distanceRemaining = moveVec.length();

    Vec2 direction = moveVec / moveVec.length();
    m_velocity = direction * m_Stats.getSpeed();

    // PROJECT 2: This is where your collision code will be called from
    std::vector < const Entity*> otherMob = checkCollision(deltaTSec);
    for (const Entity* e : otherMob) {
        processCollision(e, deltaTSec); //Cahnge velocity to account for collisions
    }

    //Distance to move by
    Vec2 offset = m_velocity * deltaTSec;
    

    // if we're moving to m_pTarget, don't move into it
    if (bMoveToTarget)
    {
        assert(m_pTarget);
        distanceRemaining -= (m_Stats.getSize() + m_pTarget->getStats().getSize()) / 2.f;
        distanceRemaining = std::max(0.f, distanceRemaining);
        
    }

    if (offset.length() <= distanceRemaining)
    {
        m_Pos += offset;
    }
    else
    {
        m_Pos += (offset/offset.length())*distanceRemaining;

        // if the destination was a waypoint, find the next one and continue movement
        if (m_pWaypoint)
        {
            m_pWaypoint = pickWaypoint();
            destPos = m_pWaypoint ? *m_pWaypoint : m_Pos;
            moveVec = destPos - m_Pos;
            Vec2 direction = moveVec / moveVec.length();
            m_velocity = direction * m_Stats.getSpeed();
            Vec2 offset = m_velocity * deltaTSec;
            m_Pos += (offset / offset.length()) * distanceRemaining;
        }
    }


}

const Vec2* Mob::pickWaypoint()
{
    float smallestDistSq = FLT_MAX;
    const Vec2* pClosest = NULL;

    for (const Vec2& pt : Game::get().getWaypoints())
    {
        // Filter out any waypoints that are behind (or barely in front of) us.
        // NOTE: (0, 0) is the top left corner of the screen
        float yOffset = pt.y - m_Pos.y;
        if ((m_bNorth && (yOffset < 1.f)) ||
            (!m_bNorth && (yOffset > -1.f)))
        {
            continue;
        }

        float distSq = m_Pos.distSqr(pt);
        if (distSq < smallestDistSq) {
            smallestDistSq = distSq;
            pClosest = &pt;
        }
    }

    return pClosest;
}


float EuclideanDistance(Vec2 a, Vec2 b) {
    return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

// PROJECT 2: 
//  1) return a vector of mobs that we're colliding with
//  2) handle collision with towers & river 
std::vector < const Entity*> Mob::checkCollision(float deltaTSec)
{
    std::vector < const Entity*> collidingMobs =  std::vector < const Entity*>();
    const Player& northPlayer = Game::get().getPlayer(true);
    Vec2 ahead = m_Pos + ((m_velocity / m_velocity.length()) * deltaTSec * 2);
    for (const Entity* pOtherMob : northPlayer.getMobs())
    {
        if (this == pOtherMob) 
        {
            continue;
        }

        // PROJECT 2: YOUR CODE CHECKING FOR A COLLISION GOES HERE
        
        if (EuclideanDistance(ahead,pOtherMob->getPosition()) < m_Stats.getSize()+pOtherMob->getStats().getSize()) {
            collidingMobs.push_back(pOtherMob);
            //std::cout << "Collision detected";
        }

    }

    const Player& southPlayer = Game::get().getPlayer(false);
    for (const Entity* pOtherMob : southPlayer.getMobs())
    {
        if (this == pOtherMob)
        {
            continue;
        }

        // PROJECT 2: YOUR CODE CHECKING FOR A COLLISION GOES HERE
        if (EuclideanDistance(ahead, pOtherMob->getPosition()) < m_Stats.getSize() + pOtherMob->getStats().getSize()) {
            collidingMobs.push_back(pOtherMob);
            std::cout << "Collision detected";
        }
    }

    return collidingMobs;
}

void Mob::processCollision(const Entity* otherMob, float deltaTSec)
{
    // PROJECT 2: YOUR COLLISION HANDLING CODE GOES HERE
    Vec2 steeringForce;
    Vec2 velocityChange;
    if (m_Stats.getMass() < otherMob->getData().m_Stats.getMass()) {
        Vec2 ahead = m_Pos + ((m_velocity / m_velocity.length()) * deltaTSec*2);
        //Steering force = projected postition - obstacle position
        steeringForce = ahead - otherMob->getData().m_Position;
        steeringForce.normalize();
        //Velocity = acceleration * time
        velocityChange = steeringForce * deltaTSec*1000; 
        //Add change in velocity to current velocity
        m_velocity += velocityChange; 
        //If speed is exceeding max speed set it to max speed
        if (m_velocity.length() > m_Stats.getSpeed()) {
            m_velocity.normalize();
            m_velocity *= m_Stats.getSpeed();
        }
    
    }

}

