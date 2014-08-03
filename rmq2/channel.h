/*
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MIT
 *
 * Portions created by Alan Antonuk are Copyright (c) 2012-2014
 * Alan Antonuk. All Rights Reserved.
 *
 * Portions created by VMware are Copyright (c) 2007-2012 VMware, Inc.
 * All Rights Reserved.
 *
 * Portions created by Tony Garnock-Jones are Copyright (c) 2009-2010
 * VMware, Inc. and Tony Garnock-Jones. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * ***** END LICENSE BLOCK *****
 */

#ifndef RMQ2_CHANNEL_H
#define RMQ2_CHANNEL_H

#include <stdint.h>

#include <functional>
#include <string>

#include "rmq2/status.h"

namespace rmq2 {

class Envelope;
class Message;
class Table;

class Channel {
 public:
  enum Flags {
    kPassive = 1,
    kDurable = 2,
    kAutoDelete = 4,
    kInternal = 8,
    kExclusive = 16,
    kIfUnused = 32,
    kIfEmpty = 64,
    kMandatory = 128,
    kImmediate = 256,
    kNoLocal = 512,
    kNoAck = 1024,
    kMultiple = 2048,
    kRequeue = 4096
  };

  constexpr char* kDirectExchange = "direct";
  constexpr char* kFanoutExchange = "fanout";
  constexpr char* kTopicExchange = "topic";
  constexpr char* kHeaderExchange = "header";

  /**
   * Declare an exchange on the broker.
   *
   * \param [in] name name of the exchange to declare.
   * \param [in] type type of the exchange. Examples include direct, fanout, topic.
   * \param [in] flags acceptable flags: kPassive, kDurable, kAutoDelete, kInternal
   * \param [in] args additional args to pass to the broker.
   * \return Status
   */
  virtual Status DeclareExchange(const std::string& name,
                                 const std::string& type, Flags flags,
                                 const Table& args);

  /**
   * Delete an exchange on the broker.
   *
   * Remove an exchange on the broker if it exists.
   * Note: as of RabbitMQ 3.3.x exchanges are indempotent. Attempting to delete
   * an exchange that is not declared will not result in an error.
   *
   * \param [in] name the name of the exchange to delete.
   * \param [in] flags acceptable flags kIfUnused
   * \return Status
   */
  virtual Status DeleteExchange(const std::string& name, Flags flags);

  /**
   * Bind an exchange to another exchange.
   *
   * \param [in] destination exchange to route messages to.
   * \param [in] source exchange to route messages from.
   * \param [in] routing_key the routing key to match when making the routing
   *     decision.
   * \param [in] args additional args to pass to the broker.
   * \return Status
   */
  virtual Status BindExchange(const std::string& destination,
                              const std::string& source,
                              const std::string& routing_key,
                              const Table& args);

  /**
   * Unbind an exchange from an another exchange.
   *
   * \param [in] destination exchange messages are being routed to.
   * \param [in] source exchange to route messages from.
   * \param [in] routing_key the routing key to match when making the routing
   *     decision.
   * \param [in] args additional arguments to pass to the broker.
   * \return Status
   */
  virtual Status UnbindExchange(const std::string& destination,
                                const std::string& source,
                                const std::string& routing_key,
                                const Table& args);

  struct QueueInfo {
    std::string name;         /**< name of the queue */
    uint64_t message_count;   /**< number of messages in the queue. */
    uint32_t consumer_count;  /**< number of consumers attached to the queue. */
  };
  /**
   * Declare a queue.
   *
   * \param [in] name the name of the queue to declare. Note that an empty
   *     string will let the broker generate a queue name.
   * \param [in] flags acceptable flags: kPassive, kDurable, kExclusive,
   *     kAutoDelete.
   * \param [in] args additional arguments to pass to the broker.
   * \param [out] info information returned by the broker when declaring the
   *     queue.
   * \return Status
   */
  virtual Status DeclareQueue(const std::string& name, Flags flags,
                              const Table& args, QueueInfo* info);

  /**
   * Delete a queue.
   *
   * \param [in] name the name of the queue.
   * \param [in] flags acceptable flags: kIfUnused, kIfEmpty.
   * \param [out] message_count the number of messages deleted.
   * \return Status
   */
  virtual Status DeleteQueue(const std::string& name, Flags flags,
                             uint64_t* message_count);

  /**
   * Bind a queue to an exchange.
   *
   * \param [in] queue name of the queue to bind.
   * \param [in] exchange name of the exchange to bind to.
   * \param [in] routing_key the routing key to bind over.
   * \param [in] args additional arguments to pass to the broker.
   * \return Status
   */
  virtual Status BindQueue(const std::string& queue,
                           const std::string& exchange,
                           const std::string& routing_key, const Table& args);

  /**
   * Unbind a queue from an exchange.
   *
   * \param [in] queue name of the queue to unbind.
   * \param [in] exchange name of the exchange to unbind to.
   * \param [in] routing_key the routing key to unbind over.
   * \param [in] args additional arguments to send to the broker.
   * \return Status
   */
  virtual Status UnbindQueue(const std::string& queue,
                             const std::string& exchange,
                             const std::string& routing_key, const Table& args);

  /**
   * Delete all the messages currently in a queue.
   *
   * \param [in] queue name of the queue.
   * \param [out] message_count number of messages deleted during queue purge.
   * \return Status
   */
  virtual Status PurgeQueue(const std::string& queue, uint64_t* message_count);

  /**
   * Publish a message.
   *
   * This function does not wait for acknowledgement from the broker that the
   * message has been successfully published.
   *
   * \param [in] exchange name of the exchange to publish to.
   * \param [in] routing_key rk to route message with.
   * \param [in] flags acceptable flags are kImmediate and kMandatory
   * \param [in] message the message to publish.
   * \return Status
   */
  virtual Status Publish(const std::string& exchange,
                         const std::string& routing_key, Flags flags,
                         const Message message);

  typedef std::function<void(uint64_t)> PublishConfirm;
  /**
   * Add a publisher confirm.
   *
   * This puts the channel in publisher confirm mode if it isn't already. The
   * PublisherConfirmCallback will be called when the broker has dealt with a
   * message.
   *
   * \param [in] func callback.
   * \return Status
   */
  virtual Status AddPublishConfirm(PublishConfirm &func);


  typedef std::function<void(const std::string&, Envelope)> Consumer;
  /**
   * Start a consumer on a queue.
   *
   * \param [in] queue the name of the queue.
   * \param [in] tag an identifier for the consumer. If you leave this blank,
   *    the broker will generate one and it'll be returned by the tag_out
   *    parameter.
   * \param [in] flags acceptable flags are kNoLocal, kNoAck, kExclusive.
   * \param [in] args additional arguments to pass to the broker.
   * \param [in] consumer a function to call when a message is consumed. Note
   *     that this function may not be called on the thread that its declared
   *     in.
   * \param [out] tag_out the consumer tag that the broker returns.
   * \return Status
   */
  virtual Status Consume(const std::string& queue, const std::string& tag,
                         Flags flags, Table& args, Consumer consumer,
                         std::string* tag_out);

  /**
   * Cancel a consumer.
   *
   * \param [in] tag the consumer-tag of the consumer to cancel.
   * \return Status
   */
  virtual Status CancelConsumer(const std::string &tag);

  struct GetInfo {
    bool is_empty;          /**< true if queue is empty. Envelope is empty. */
    Envelope envelope;      /**< the message. */
    uint64_t message_count; /**< the number of messages left in the queue. */
  };
  /**
   * Synchronously get a message from a queue.
   *
   * \param [in] queue the name of the queue to get a message from.
   * \param [in] flags acceptable flags are: kNoAck.
   * \param [out] result the message if the queue has a message.
   * \return Status
   */
  virtual Status Get(const std::string& queue, Flags flags, GetInfo* result);

  /**
   * Acknowledge a message.
   *
   * \param [in] delivery_tag
   * \param [in] flags acceptable flags are: kMultiple.
   * \return Status
   */
  virtual Status Ack(uint64_t delivery_tag, Flags flags);

  /**
   * Negatively acknowledge a message.
   *
   * \param [in] delivery_tag
   * \param [in] flags acceptable flags are: kMultiple, kRequeue.
   * \return Status
   */
  virtual Status Nack(uint64_t delivery_tag, Flags flags);

  /**
   * Redeliver all unacknowledged messages on this channel.
   *
   * \param [in] flags acceptable flags are kRequeue.
   * \return Status
   */
  virtual Status Recover(Flags flags);

  /**
   * Set prefetch limits for the channel.
   *
   * \param [in] size size of outstanding messages in bytes.
   * \param [in] count number of outstanding message.
   * \return Status
   */
  virtual Status Qos(uint32_t size, uint16_t count);

  /**
   * Start a transaction.
   *
   * \return Status
   */
  virtual Status TransactionBegin();

  /**
   * Commit a transaction.
   *
   * \return Status
   */
  virtual Status TransactionCommit();

  /**
   * Rollback a transaction.
   *
   * \return Status
   */
  virtual Status TransactionRollback();
};
}  // namespace rmq2
#endif  /* RMQ2_CHANNEL_H */
