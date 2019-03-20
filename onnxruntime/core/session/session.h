// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <string>
#include <unordered_map>

#include "core/common/common.h"
#include "core/common/status.h"
#include "core/framework/framework_common.h"
#include "core/graph/basic_types.h"
#include "core/common/logging/logging.h"

namespace onnxruntime {  // forward declarations
class GraphTransformer;
}  // namespace onnxruntime

namespace ONNX_NAMESPACE {
class ModelProto;
}  // namespace ONNX_NAMESPACE

struct OrtCustomOpDomain {
  std::string domain_;
  std::vector<OrtCustomOp*> custom_ops_;
};

namespace onnxruntime {
class IExecutionProvider;  // forward decl
class IOBinding;

class CustomRegistry;

namespace logging {
class LoggingManager;
}

/**
  * Configuration information for a session.
  */
struct SessionOptions {
  //int num_threads; // not used now until we re-introduce threadpools for async execution
  bool enable_sequential_execution = true;  // TODO: should we default to sequential execution?

  // enable profiling for this session.
  bool enable_profiling = false;

  // enable the memory arena on CPU
  // Arena may pre-allocate memory for future usage.
  // set this option to false if you don't want it.
  bool enable_cpu_mem_arena = true;

  // the prefix of the profile file. The current time will be appended to the file name.
  std::basic_string<ORTCHAR_T> profile_file_prefix = ORT_TSTR("onnxruntime_profile_");

  std::string session_logid;                 ///< logger id to use for session output
  unsigned session_log_verbosity_level = 0;  ///< applies to session load, initialization, etc

  unsigned max_num_graph_transformation_steps = 5;  // TODO choose a good default here?

  // set graph optimization level
  // allowed levels are 0, 1, 2
  // 0 -> disable all optimizations
  // 1 -> enable basic optimizations
  // 2 -> enable all optimizations
  unsigned graph_optimization_level = 0;

  // How many threads in the session thread pool.
  int session_thread_pool_size = 0;
};

/**
  * Pre-defined and custom metadata about the model.
  */
struct ModelMetadata {
  std::string producer_name;
  std::string graph_name;
  std::string domain;
  std::string description;
  int64_t version;
  std::unordered_map<std::string, std::string> custom_metadata_map;
};

/**
  * @brief This is the main class used to Run a model.
  * Sample simple usage:
  *  CPUExecutionProviderInfo epi;
  *  ProviderOption po{"CPUExecutionProvider", epi};
  *  SessionOptions so(vector<ProviderOption>{po});
  *  auto session_object = Session::Create(so); // session_object is a unique_ptr
  *  common::Status status = session_object->Load(MODEL_URI);
  *  common::Status status = session_object->Initialize();
  *
  *  NameMLValMap feeds;
  *  feeds.insert({});
  *  ...
  *  std::vector<std::string> output_names;
  *  output_names.insert(...);
  *  ...
  *  std::vector<MLValue> fetches;
  *  common::Status status = session_object->Run(run_options, feeds, output_names, &fetches);
  *  process the output here...
  */

class Session {
 public:
  /**
    Create a new Session
    @param session_options Session options.
    @param logging_manager
    Optional logging manager instance that will enable per session logger output using
    session_options.session_logid as the logger id in messages.
    If nullptr, the default LoggingManager MUST have been created previously as it will be used
    for logging. This will use the default logger id in messages.
    See core/common/logging/logging.h for details, and how LoggingManager::DefaultLogger works.
    */
  static std::unique_ptr<Session> Create(const SessionOptions& session_options,
                                         logging::LoggingManager* logging_manager = nullptr);

  virtual ~Session() = default;

  /**
    * Register an execution provider. If you've one to register, call this before invoking Initialize().
    * The order of invocation indicates the preference order as well. In other words call this method 
    * on your most preferred execution provider first followed by the less preferred ones.
    * Calling this API is optional in which case onnxruntime will use its internal CPU execution provider.
    * @return OK if success.
    */
  virtual common::Status RegisterExecutionProvider(std::unique_ptr<IExecutionProvider> p_exec_provider) = 0;

  /**
    * Register a graph transformer. If you've one to register, call this before invoking Initialize().
    * Calling this API is optional.
    * @param[in] - providers Optional. If providers is non-empty this transformer will only to 
      applied to nodes which are assigned to given providers.
    * @param[in] - level Optional. Level to which this transformer should be registered. Default is set to 2.
    * @return OK if success.
    */
  virtual common::Status RegisterGraphTransformer(std::unique_ptr<onnxruntime::GraphTransformer> p_graph_transformer,
                                                  const std::vector<std::string>& providers = {},
                                                  const uint32_t& level = 2) = 0;

  /**
    * Enable a custom set of transformers. Call this before invoking Initialize().
    * Calling this API is optional.
    * When this list is provided ORT ignores the levels set in session options.
    * @return OK if success.
    */
  virtual common::Status AddCustomTransformerList(const std::vector<std::string>& transformers_to_enable) = 0;

  virtual common::Status AddCustomOpDomains(const std::vector<OrtCustomOpDomain*>& ops) = 0;

  /**
    * Register a custom registry for operator schema and kernels.  If you've one to register, 
    * call this before invoking Initialize().
    * The order of invocation indicates the reversed preference order: Register your most 
    * preferred registry at the end.
    * Calling this API is optional.
    * @return OK if success.
    */
  virtual common::Status RegisterCustomRegistry(std::shared_ptr<CustomRegistry> custom_registry) = 0;

  /**
    * Load an ONNX model.
    * @param model_uri absolute path of the model file.
    * @return OK if success.
    */
  virtual common::Status Load(const std::string& model_uri) = 0;
#ifdef _WIN32
  virtual common::Status Load(const std::wstring& model_uri) = 0;
#endif
  /**
    * Load an ONNX model.
    * @param istream object of the model.
    * @return OK if success.
    */
  virtual common::Status Load(std::istream& model_istream) = 0;

  /**
    * Initializes a previously loaded model. Initialization includes but is not
    * limited to graph transformations, construction of kernels, etc.
    * This method assumes that a method has been loaded previously.
    * @return OK if success
    */
  virtual common::Status Initialize() = 0;

  virtual common::Status Run(const RunOptions& run_options,
                             const std::vector<std::string>& feed_names,
                             const std::vector<MLValue>& feeds,
                             const std::vector<std::string>& output_names,
                             std::vector<MLValue>* p_fetches) = 0;

  /**
    * Run a pre-loaded and pre-intialized model.
    * Multiple threads are allowed to run this function; hence its thread-safe.
    * @param feeds named inputs owned by client code and should not be changed during
    *        execution of this function.
    * @param output_names output names
    * @param p_fetches output values in the order specified by output_names.
    *        This should not be changed during execution of this function.
    * @return OK if success.
    */
  virtual common::Status Run(const NameMLValMap& feeds,
                             const std::vector<std::string>& output_names,
                             std::vector<MLValue>* p_fetches) = 0;

  /**
    * See Run(const NameMLValMap& feeds, const std::vector<std::string>& output_names, std::vector<MLValue>* p_fetches)
    * for details.
    * @param run_options use this to tune the Run call to your needs.
    */
  virtual common::Status Run(const RunOptions& run_options,
                             const NameMLValMap& feeds,
                             const std::vector<std::string>& output_names,
                             std::vector<MLValue>* p_fetches) = 0;

  /**
  * Creates a new binding object for binding inputs and outputs.
  * @param provider_type specifies the location where the inputs need to be potentially copied. 
  * See IOBinding class for more info.
  */
  virtual common::Status NewIOBinding(std::unique_ptr<IOBinding>* io_binding) = 0;

  virtual common::Status Run(const RunOptions& run_options, IOBinding& io_binding) = 0;
  virtual common::Status Run(IOBinding& io_binding) = 0;

  /**
    * @return pair.first = OK; FAIL otherwise. pair.second is non-NULL when pair.first = OK.
    * @note lifetime of the returned pointer is valid as long as the Session object is live.
    */
  virtual std::pair<common::Status, const ModelMetadata*> GetModelMetadata() const = 0;

  /**
    * Get all input definitions of the model. This does not include weights. Use this
    * to get the name/type/shapes of the inputs.
    * @return pair.first = OK; FAIL otherwise. pair.second is non-NULL when pair.first = OK.
    * @note lifetime of the returned pointer is valid as long as the Session object is live.
    */
  virtual std::pair<common::Status, const InputDefList*> GetModelInputs() const = 0;

  /**
    * Get all output definitions of the model. Use this to get the name/type/shapes of the outputs.
    * @return pair.first = OK; FAIL otherwise. pair.second is non-NULL when pair.first = OK.
    * @note lifetime of the returned pointer is valid as long as the Session object is live.
    */
  virtual std::pair<common::Status, const OutputDefList*> GetModelOutputs() const = 0;

  /**
    * Get the current number of in-progress concurrent Run calls.
    */
  virtual int GetCurrentNumRuns() const = 0;

  /**
    * Start profiling on this inference session. This simply turns on profiling events to be 
    * recorded. A corresponding EndProfiling has to follow to write profiling data to a file.
    *@param file_prefix is the prefix of the profile file. It can include a directory path. 
    */
  virtual void StartProfiling(const std::string& file_prefix) = 0;
#ifdef _WIN32
  virtual void StartProfiling(const std::wstring& file_prefix) = 0;
#endif
  /**
    * Start profiling on this inference session. This simply turns on profiling events to be
    * recorded. A corresponding EndProfiling has to follow to send profiling events through the logger's ISink.
    *@param logger_ptr is pointer to the logger where profiling events will be sent to.
    */
  virtual void StartProfiling(const logging::Logger* logger_ptr) = 0;

  /**
    * Write captured profile events in chromium format.
    @return the name of the profile file.
    */
  virtual std::string EndProfiling() = 0;

 protected:
  /**
    * Load an ONNX model.
    * @param protobuf object corresponding to the model file. model_proto will be copied by the API.
    * @return OK if success.
    */
  virtual common::Status Load(const ONNX_NAMESPACE::ModelProto& model_proto) = 0;

  /**
    * Load an ONNX model.
    * @param protobuf object corresponding to the model file. This is primarily to support large models.
    * @return OK if success.
    */
  virtual common::Status Load(std::unique_ptr<ONNX_NAMESPACE::ModelProto> p_model_proto) = 0;

  // A trivial class for sub class's constructor.
  // to enforce Session::Create() being the only way for construction.
  class SubClassConstructorCookie {};
};
}  // namespace onnxruntime
