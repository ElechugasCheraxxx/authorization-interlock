package blackboard

import (
	"context"
	"encoding/json"
	"fmt"
	"github.com/redis/go-redis/v9"
	"log"
)

type Blackboard struct {
	client *redis.Client
	ctx    context.Context
}

func NewBlackboard(addr string) (*Blackboard, error) {
	rdb := redis.NewClient(&redis.Options{
		Addr: addr,
	})

	ctx := context.Background()
	if err := rdb.Ping(ctx).Err(); err != nil {
		return nil, fmt.Errorf("failed to connect to Redis: %v", err)
	}

	return &Blackboard{
		client: rdb,
		ctx:    ctx,
	}, nil
}

// PublishTacticalPlan publica un plan de ataque en el Redis Stream "blackboard:tactical_plans"
func (b *Blackboard) PublishTacticalPlan(plan interface{}) error {
	data, err := json.Marshal(plan)
	if err != nil {
		return err
	}

	err = b.client.XAdd(b.ctx, &redis.XAddArgs{
		Stream: "blackboard:tactical_plans",
		Values: map[string]interface{}{
			"artifact": string(data),
			"type":    "PLAN_CREATED",
		},
	}).Err()

	if err != nil {
		log.Printf("[Blackboard] Error publishing plan: %v", err)
		return err
	}

	return nil
}

// PublishMissionUpdate publica actualizaciones de estado de las subtasks
func (b *Blackboard) PublishMissionUpdate(subtaskID string, status string) error {
	return b.client.XAdd(b.ctx, &redis.XAddArgs{
		Stream: "blackboard:mission_updates",
		Values: map[string]interface{}{
			"subtask_id": subtaskID,
			"status":     status,
		},
	}).Err()
}
