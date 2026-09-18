#pragma once
// generated from MCP schema 2026-07-28, do not edit
// See https://modelcontextprotocol.io/docs/2026-07-28/getting-started/intro
// See https://github.com/modelcontextprotocol/modelcontextprotocol/tree/main/schema
#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace mcp::v2026_07_28 {

struct JsonValue final {
    using array = std::vector<JsonValue>;
    using object = std::map<std::string, JsonValue>;
    std::variant<std::nullptr_t, bool, std::int64_t, double, std::string, array, object> data;
};


struct Error final {
    /**
     * The error type that occurred.
     */
    double code;
    /**
     * Additional information about the error. The value of this member is defined by the sender
     * (e.g. detailed error information, nested errors etc.).
     */
    JsonValue data;
    /**
     * A short description of the error. The message SHOULD be limited to a concise single
     * sentence.
     */
    std::string message;
};

using RequestId = std::variant<double, std::string>;

enum class jsonrpc : int { the20 };

/**
 * Optional specifier for the theme this icon is designed for. `"light"` indicates
 * the icon is designed to be used with a light background, and `"dark"` indicates
 * the icon is designed to be used with a dark background.
 *
 * If not provided, the client should assume the icon can be used with any theme.
 */
enum class theme : int { dark, light };

/**
 * An optionally-sized icon that can be displayed in a user interface.
 */
struct Icon final {
    /**
     * Optional MIME type override if the source MIME type is missing or generic.
     * For example: `"image/png"`, `"image/jpeg"`, or `"image/svg+xml"`.
     */
    std::string mimeType;
    /**
     * Optional array of strings that specify sizes at which the icon can be used.
     * Each string should be in WxH format (e.g., `"48x48"`, `"96x96"`) or `"any"` for scalable
     * formats like SVG.
     *
     * If not provided, the client should assume that the icon can be used at any size.
     */
    std::vector<std::string> sizes;
    /**
     * A standard URI pointing to an icon resource. May be an HTTP/HTTPS URL or a
     * `data:` URI with Base64-encoded image data.
     *
     * Consumers SHOULD take steps to ensure URLs serving icons are from the
     * same domain as the client/server or a trusted domain.
     *
     * Consumers SHOULD take appropriate precautions when consuming SVGs as they can contain
     * executable JavaScript.
     */
    std::string src;
    /**
     * Optional specifier for the theme this icon is designed for. `"light"` indicates
     * the icon is designed to be used with a light background, and `"dark"` indicates
     * the icon is designed to be used with a dark background.
     *
     * If not provided, the client should assume the icon can be used with any theme.
     */
    theme theme;
};

/**
 * Identifies the server software producing the response. Servers SHOULD
 * include this field on every response unless specifically configured not
 * to do so.
 *
 * The {@link Implementation} schema requires `name` and `version`; other
 * fields are optional.
 *
 * The value is self-reported by the server and is not verified by the
 * protocol. It is intended for display, logging, and debugging. Clients
 * SHOULD NOT use it to change their behavior, and SHOULD NOT rely on it for
 * security decisions.
 *
 * Describes the MCP implementation.
 *
 * Identifies the client software making the request. Clients SHOULD
 * include this field on every request unless specifically configured not
 * to do so.
 *
 * The {@link Implementation} schema requires `name` and `version`; other
 * fields are optional.
 *
 * The value is self-reported by the client and is not verified by the
 * protocol. It is intended for display, logging, and debugging. Servers
 * SHOULD NOT use it to change their behavior, and SHOULD NOT rely on it for
 * security decisions.
 */
struct Implementation final {
    /**
     * An optional human-readable description of what this implementation does.
     *
     * This can be used by clients or servers to provide context about their purpose
     * and capabilities. For example, a server might describe the types of resources
     * or tools it provides, while a client might describe its intended use case.
     */
    std::string description;
    /**
     * Optional set of sized icons that the client can display in a user interface.
     *
     * Clients that support rendering icons MUST support at least the following MIME types:
     * - `image/png` - PNG images (safe, universal compatibility)
     * - `image/jpeg` (and `image/jpg`) - JPEG images (safe, universal compatibility)
     *
     * Clients that support rendering icons SHOULD also support:
     * - `image/svg+xml` - SVG images (scalable but requires security precautions)
     * - `image/webp` - WebP images (modern, efficient format)
     */
    std::vector<Icon> icons;
    /**
     * Intended for programmatic or logical use, but used as a display name in past specs or
     * fallback (if title isn't present).
     */
    std::string name;
    /**
     * Intended for UI and end-user contexts 
 optimized to be human-readable and easily
     * understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for {@link Tool},
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::string title;
    /**
     * The version of this implementation.
     */
    std::string version;
    /**
     * An optional URL of the website for this implementation.
     */
    std::string websiteUrl;
};

/**
 * Extends {@link MetaObject} with additional result-specific fields. All key naming rules
 * from `MetaObject` apply.
 */
struct ResultMetaObject final {
    /**
     * Identifies the server software producing the response. Servers SHOULD
     * include this field on every response unless specifically configured not
     * to do so.
     *
     * The {@link Implementation} schema requires `name` and `version`; other
     * fields are optional.
     *
     * The value is self-reported by the server and is not verified by the
     * protocol. It is intended for display, logging, and debugging. Clients
     * SHOULD NOT use it to change their behavior, and SHOULD NOT rely on it for
     * security decisions.
     */
    Implementation iomodelcontextprotocolserverInfo;
};

/**
 * Common result fields.
 */
struct ClientResult final {
    /**
     * Extends {@link MetaObject} with additional result-specific fields. All key naming rules
     * from `MetaObject` apply.
     */
    ResultMetaObject _meta;
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    std::string resultType;
};

/**
 * Refers to any valid JSON-RPC object that can be decoded off the wire, or encoded to be
 * sent.
 *
 * A request that expects a response.
 *
 * A notification which does not expect a response.
 *
 * A successful (non-error) response to a request.
 *
 * A response to a request that indicates an error occurred.
 */
struct JSONRPCMessage final {
    /**
     * A uniquely identifying ID for a request in JSON-RPC.
     */
    RequestId id;
    jsonrpc jsonrpc;
    std::string method;
    std::map<std::string, JsonValue> params;
    ClientResult result;
    Error error;
};

struct Request final {
    std::string method;
    std::map<std::string, JsonValue> params;
};

struct Notification final {
    std::string method;
    std::map<std::string, JsonValue> params;
};

/**
 * A request that expects a response.
 */
struct JSONRPCRequest final {
    RequestId id;
    jsonrpc jsonrpc;
    std::string method;
    std::map<std::string, JsonValue> params;
};

/**
 * A notification which does not expect a response.
 */
struct JSONRPCNotification final {
    jsonrpc jsonrpc;
    std::string method;
    std::map<std::string, JsonValue> params;
};

/**
 * A successful (non-error) response to a request.
 */
struct JSONRPCResultResponse final {
    RequestId id;
    jsonrpc jsonrpc;
    ClientResult result;
};

/**
 * A response to a request that indicates an error occurred.
 */
struct JSONRPCErrorResponse final {
    Error error;
    /**
     * A uniquely identifying ID for a request in JSON-RPC.
     */
    RequestId id;
    jsonrpc jsonrpc;
};

/**
 * A response to a request, containing either the result or error.
 *
 * A successful (non-error) response to a request.
 *
 * A response to a request that indicates an error occurred.
 */
struct JSONRPCResponse final {
    /**
     * A uniquely identifying ID for a request in JSON-RPC.
     */
    RequestId id;
    jsonrpc jsonrpc;
    ClientResult result;
    Error error;
};

/**
 * A JSON-RPC error indicating that invalid JSON was received by the server. This error is
 * returned when the server cannot parse the JSON text of a message.
 */
struct ParseError final {
    /**
     * The error type that occurred.
     */
    double code;
    /**
     * Additional information about the error. The value of this member is defined by the sender
     * (e.g. detailed error information, nested errors etc.).
     */
    JsonValue data;
    /**
     * A short description of the error. The message SHOULD be limited to a concise single
     * sentence.
     */
    std::string message;
};

/**
 * A JSON-RPC error indicating that the request is not a valid request object. This error is
 * returned when the message structure does not conform to the JSON-RPC 2.0 specification
 * requirements for a request (e.g., missing required fields like `jsonrpc` or `method`, or
 * using invalid types for these fields).
 */
struct InvalidRequestError final {
    /**
     * The error type that occurred.
     */
    double code;
    /**
     * Additional information about the error. The value of this member is defined by the sender
     * (e.g. detailed error information, nested errors etc.).
     */
    JsonValue data;
    /**
     * A short description of the error. The message SHOULD be limited to a concise single
     * sentence.
     */
    std::string message;
};

/**
 * A JSON-RPC error indicating that the requested method does not exist or is not
 * available.
 *
 * In MCP, a server returns this error when a client invokes a method the server does not
 * implement 
 either a genuinely unknown method, or one gated behind a server capability
 * the server did not advertise (e.g., calling `prompts/list` when the `prompts` capability
 * was not advertised).
 *
 * A request that requires a client capability the client did not declare is signalled
 * instead by {@link MissingRequiredClientCapabilityError} (`-32021`).
 */
struct MethodNotFoundError final {
    /**
     * The error type that occurred.
     */
    double code;
    /**
     * Additional information about the error. The value of this member is defined by the sender
     * (e.g. detailed error information, nested errors etc.).
     */
    JsonValue data;
    /**
     * A short description of the error. The message SHOULD be limited to a concise single
     * sentence.
     */
    std::string message;
};

/**
 * A JSON-RPC error indicating that the method parameters are invalid or malformed.
 *
 * In MCP, this error is returned in various contexts when request parameters fail
 * validation:
 *
 * - **Tools**: Unknown tool name or invalid tool arguments
 * - **Prompts**: Unknown prompt name or missing required arguments
 * - **Pagination**: Invalid or expired cursor values
 * - **Logging**: Invalid log level
 * - **Elicitation**: Server requests an elicitation mode not declared in client
 * capabilities
 * - **Sampling**: Missing tool result or tool results mixed with other content
 */
struct InvalidParamsError final {
    /**
     * The error type that occurred.
     */
    double code;
    /**
     * Additional information about the error. The value of this member is defined by the sender
     * (e.g. detailed error information, nested errors etc.).
     */
    JsonValue data;
    /**
     * A short description of the error. The message SHOULD be limited to a concise single
     * sentence.
     */
    std::string message;
};

/**
 * A JSON-RPC error indicating that an internal error occurred on the receiver. This error
 * is returned when the receiver encounters an unexpected condition that prevents it from
 * fulfilling the request.
 */
struct InternalError final {
    /**
     * The error type that occurred.
     */
    double code;
    /**
     * Additional information about the error. The value of this member is defined by the sender
     * (e.g. detailed error information, nested errors etc.).
     */
    JsonValue data;
    /**
     * A short description of the error. The message SHOULD be limited to a concise single
     * sentence.
     */
    std::string message;
};

struct HeaderMismatchError_error final {
    double code;
};

/**
 * Returned when a server rejects a request because the values in the HTTP
 * headers do not match the corresponding values in the request body, or
 * because required headers are missing or malformed. For HTTP, the response
 * status code MUST be `400 Bad Request`.
 */
struct HeaderMismatchError final {
    HeaderMismatchError_error error;
    /**
     * A uniquely identifying ID for a request in JSON-RPC.
     */
    RequestId id;
    jsonrpc jsonrpc;
};

struct Purple_data final {
    /**
     * The protocol version that was requested by the client.
     */
    std::string requested;
    /**
     * Protocol versions the server supports. The client should choose a
     * mutually supported version from this list and retry.
     */
    std::vector<std::string> supported;
};

struct UnsupportedProtocolVersionError_error final {
    double code;
    Purple_data data;
};

/**
 * Returned when the request's protocol version is unknown to the server or
 * unsupported (e.g., a known experimental or draft version the server has
 * chosen not to implement). For HTTP, the response status code MUST be
 * `400 Bad Request`.
 */
struct UnsupportedProtocolVersionError final {
    UnsupportedProtocolVersionError_error error;
    /**
     * A uniquely identifying ID for a request in JSON-RPC.
     */
    RequestId id;
    jsonrpc jsonrpc;
};


/**
 * Present if the client supports elicitation from the server.
 */
struct elicitation final {
    std::map<std::string, JsonValue> form;
    std::map<std::string, JsonValue> url;
};

/**
 * Present if the client supports sampling from an LLM.
 */
struct sampling final {
    /**
     * Whether the client supports context inclusion via `includeContext` parameter.
     * If not declared, servers SHOULD only use `includeContext: "none"` (or omit it).
     */
    std::map<std::string, JsonValue> context;
    /**
     * Whether the client supports tool use via `tools` and `toolChoice` parameters.
     */
    std::map<std::string, JsonValue> tools;
};

/**
 * The client's capabilities for this specific request. Required.
 *
 * Capabilities are declared per-request rather than once at initialization;
 * an empty object means the client supports no optional capabilities.
 * Servers MUST NOT infer capabilities from prior requests.
 *
 * Capabilities a client may support. Known capabilities are defined here, in this schema,
 * but this is not a closed set: any client can define its own, additional capabilities.
 *
 * The capabilities the server requires from the client to process this request.
 */
struct ClientCapabilities final {
    /**
     * Present if the client supports elicitation from the server.
     */
    elicitation elicitation;
    /**
     * Experimental, non-standard capabilities that the client supports.
     */
    std::map<std::string, std::map<std::string, JsonValue>> experimental;
    /**
     * Optional MCP extensions that the client supports. Keys are extension identifiers
     * (e.g., "io.modelcontextprotocol/oauth-client-credentials"), and values are
     * per-extension settings objects. An empty object indicates support with no settings.
     *
     * Keys MUST follow the {@link MetaObject`_meta` key naming rules}, with a
     * mandatory prefix.
     */
    std::map<std::string, std::map<std::string, JsonValue>> extensions;
    /**
     * Present if the client supports listing roots.
     */
    std::map<std::string, JsonValue> roots;
    /**
     * Present if the client supports sampling from an LLM.
     */
    sampling sampling;
};

struct Fluffy_data final {
    /**
     * The capabilities the server requires from the client to process this request.
     */
    ClientCapabilities requiredCapabilities;
};

struct MissingRequiredClientCapabilityError_error final {
    double code;
    Fluffy_data data;
};

/**
 * Returned when processing a request requires a capability the client did not
 * declare in `clientCapabilities`. For HTTP, the response status code MUST be
 * `400 Bad Request`.
 */
struct MissingRequiredClientCapabilityError final {
    MissingRequiredClientCapabilityError_error error;
    /**
     * A uniquely identifying ID for a request in JSON-RPC.
     */
    RequestId id;
    jsonrpc jsonrpc;
};

enum class InputRequest_method : int { elicitationcreate, rootslist, samplingcreateMessage };

/**
 * Represents the contents of a `_meta` field, which clients and servers use to attach
 * additional metadata to their interactions.
 *
 * Certain key names are reserved by MCP for protocol-level metadata; implementations MUST
 * NOT make assumptions about values at these keys. Additionally, specific schema
 * definitions may reserve particular names for purpose-specific metadata, as declared in
 * those definitions.
 *
 * Valid keys have two segments:
 *
 * **Prefix:**
 * - Optional 
 if specified, MUST be a series of _labels_ separated by dots (`.`), followed
 * by a slash (`/`).
 * - Labels MUST start with a letter and end with a letter or digit. Interior characters may
 * be letters, digits, or hyphens (`-`).
 * - Implementations SHOULD use reverse DNS notation (e.g., `com.example/` rather than
 * `example.com/`).
 * - Any prefix where the second label is `modelcontextprotocol` or `mcp` is **reserved**
 * for MCP use. For example: `io.modelcontextprotocol/`, `dev.mcp/`,
 * `org.modelcontextprotocol.api/`, and `com.mcp.tools/` are all reserved. However,
 * `com.example.mcp/` is NOT reserved, as the second label is `example`.
 *
 * **Name:**
 * - Unless empty, MUST start and end with an alphanumeric character (`[a-z0-9A-Z]`).
 * - Interior characters may be alphanumeric, hyphens (`-`), underscores (`_`), or dots
 * (`.`).
 *
 * Optional metadata about the tool use. Clients SHOULD preserve this field when
 * including tool uses in subsequent sampling requests to enable caching optimizations.
 *
 * Optional metadata about the tool result. Clients SHOULD preserve this field when
 * including tool results in subsequent sampling requests to enable caching optimizations.
 */
struct MetaObject final {
};

/**
 * A request to include context from one or more MCP servers (including the caller), to be
 * attached to the prompt.
 * The client MAY ignore this request.
 *
 * Default is `"none"`. The values `"thisServer"` and `"allServers"` are deprecated
 * (SEP-2596): servers SHOULD
 * omit this field or use `"none"`, and SHOULD only use the deprecated values if the client
 * declares
 * {@link ClientCapabilities.sampling.context}.
 */
enum class includeContext : int { allServers, none, thisServer };

/**
 * The sender or recipient of messages and data in a conversation.
 */
enum class Role : int { assistant, user };

/**
 * Optional annotations for the client.
 *
 * Optional annotations for the client. The client can use annotations to inform how objects
 * are used or displayed
 */
struct Annotations final {
    /**
     * Describes who the intended audience of this object or data is.
     *
     * It can include multiple entries to indicate content useful for multiple audiences (e.g.,
     * `["user", "assistant"]`).
     */
    std::vector<Role> audience;
    /**
     * The moment the resource was last modified, as an ISO 8601 formatted string.
     *
     * Should be an ISO 8601 formatted string (e.g., "2025-01-12T15:00:58Z").
     *
     * Examples: last activity timestamp in an open file, timestamp when the resource
     * was attached, etc.
     */
    std::string lastModified;
    /**
     * Describes how important this data is for operating the server.
     *
     * A value of 1 means "most important," and indicates that the data is
     * effectively required, while 0 means "least important," and indicates that
     * the data is entirely optional.
     */
    double priority;
};

struct resource final {
    /**
     * Represents the contents of a `_meta` field, which clients and servers use to attach
     * additional metadata to their interactions.
     *
     * Certain key names are reserved by MCP for protocol-level metadata; implementations MUST
     * NOT make assumptions about values at these keys. Additionally, specific schema
     * definitions may reserve particular names for purpose-specific metadata, as declared in
     * those definitions.
     *
     * Valid keys have two segments:
     *
     * **Prefix:**
     * - Optional 
 if specified, MUST be a series of _labels_ separated by dots (`.`), followed
     * by a slash (`/`).
     * - Labels MUST start with a letter and end with a letter or digit. Interior characters may
     * be letters, digits, or hyphens (`-`).
     * - Implementations SHOULD use reverse DNS notation (e.g., `com.example/` rather than
     * `example.com/`).
     * - Any prefix where the second label is `modelcontextprotocol` or `mcp` is **reserved**
     * for MCP use. For example: `io.modelcontextprotocol/`, `dev.mcp/`,
     * `org.modelcontextprotocol.api/`, and `com.mcp.tools/` are all reserved. However,
     * `com.example.mcp/` is NOT reserved, as the second label is `example`.
     *
     * **Name:**
     * - Unless empty, MUST start and end with an alphanumeric character (`[a-z0-9A-Z]`).
     * - Interior characters may be alphanumeric, hyphens (`-`), underscores (`_`), or dots
     * (`.`).
     */
    MetaObject _meta;
    /**
     * The MIME type of this resource, if known.
     */
    std::string mimeType;
    /**
     * The text of the item. This must only be set if the item can actually be represented as
     * text (not binary data).
     */
    std::string text;
    /**
     * The URI of this resource.
     */
    std::string uri;
    /**
     * A base64-encoded string representing the binary data of the item.
     */
    std::string blob;
};

enum class ContentBlock_type : int { audio, image, resource, resource_link, text };

/**
 * Text provided to or from an LLM.
 *
 * An image provided to or from an LLM.
 *
 * Audio provided to or from an LLM.
 *
 * A resource that the server is capable of reading, included in a prompt or tool call
 * result.
 *
 * Note: resource links returned by tools are not guaranteed to appear in the results of
 * {@link ListResourcesRequestresources/list} requests.
 *
 * The contents of a resource, embedded into a prompt or tool call result.
 *
 * It is up to the client how best to render embedded resources for the benefit
 * of the LLM and/or the user.
 */
struct ContentBlock final {
    /**
     * Represents the contents of a `_meta` field, which clients and servers use to attach
     * additional metadata to their interactions.
     *
     * Certain key names are reserved by MCP for protocol-level metadata; implementations MUST
     * NOT make assumptions about values at these keys. Additionally, specific schema
     * definitions may reserve particular names for purpose-specific metadata, as declared in
     * those definitions.
     *
     * Valid keys have two segments:
     *
     * **Prefix:**
     * - Optional 
 if specified, MUST be a series of _labels_ separated by dots (`.`), followed
     * by a slash (`/`).
     * - Labels MUST start with a letter and end with a letter or digit. Interior characters may
     * be letters, digits, or hyphens (`-`).
     * - Implementations SHOULD use reverse DNS notation (e.g., `com.example/` rather than
     * `example.com/`).
     * - Any prefix where the second label is `modelcontextprotocol` or `mcp` is **reserved**
     * for MCP use. For example: `io.modelcontextprotocol/`, `dev.mcp/`,
     * `org.modelcontextprotocol.api/`, and `com.mcp.tools/` are all reserved. However,
     * `com.example.mcp/` is NOT reserved, as the second label is `example`.
     *
     * **Name:**
     * - Unless empty, MUST start and end with an alphanumeric character (`[a-z0-9A-Z]`).
     * - Interior characters may be alphanumeric, hyphens (`-`), underscores (`_`), or dots
     * (`.`).
     */
    MetaObject _meta;
    /**
     * Optional annotations for the client.
     */
    Annotations annotations;
    /**
     * The text content of the message.
     */
    std::string text;
    ContentBlock_type type;
    /**
     * The base64-encoded image data.
     *
     * The base64-encoded audio data.
     */
    std::string data;
    /**
     * The MIME type of the image. Different providers may support different image types.
     *
     * The MIME type of the audio. Different providers may support different audio types.
     *
     * The MIME type of this resource, if known.
     */
    std::string mimeType;
    /**
     * A description of what this resource represents.
     *
     * This can be used by clients to improve the LLM's understanding of available resources. It
     * can be thought of like a "hint" to the model.
     */
    std::string description;
    /**
     * Optional set of sized icons that the client can display in a user interface.
     *
     * Clients that support rendering icons MUST support at least the following MIME types:
     * - `image/png` - PNG images (safe, universal compatibility)
     * - `image/jpeg` (and `image/jpg`) - JPEG images (safe, universal compatibility)
     *
     * Clients that support rendering icons SHOULD also support:
     * - `image/svg+xml` - SVG images (scalable but requires security precautions)
     * - `image/webp` - WebP images (modern, efficient format)
     */
    std::vector<Icon> icons;
    /**
     * Intended for programmatic or logical use, but used as a display name in past specs or
     * fallback (if title isn't present).
     */
    std::string name;
    /**
     * The size of the raw resource content, in bytes (i.e., before base64 encoding or any
     * tokenization), if known.
     *
     * This can be used by Hosts to display file sizes and estimate context window usage.
     */
    double size;
    /**
     * Intended for UI and end-user contexts 
 optimized to be human-readable and easily
     * understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for {@link Tool},
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::string title;
    /**
     * The URI of this resource.
     */
    std::string uri;
    resource resource;
};

enum class SamplingMessageContentBlock_type : int { audio, image, text, tool_result, tool_use };

/**
 * Text provided to or from an LLM.
 *
 * An image provided to or from an LLM.
 *
 * Audio provided to or from an LLM.
 *
 * A request from the assistant to call a tool.
 *
 * The result of a tool use, provided by the user back to the assistant.
 */
struct SamplingMessageContentBlock final {
    /**
     * Represents the contents of a `_meta` field, which clients and servers use to attach
     * additional metadata to their interactions.
     *
     * Certain key names are reserved by MCP for protocol-level metadata; implementations MUST
     * NOT make assumptions about values at these keys. Additionally, specific schema
     * definitions may reserve particular names for purpose-specific metadata, as declared in
     * those definitions.
     *
     * Valid keys have two segments:
     *
     * **Prefix:**
     * - Optional 
 if specified, MUST be a series of _labels_ separated by dots (`.`), followed
     * by a slash (`/`).
     * - Labels MUST start with a letter and end with a letter or digit. Interior characters may
     * be letters, digits, or hyphens (`-`).
     * - Implementations SHOULD use reverse DNS notation (e.g., `com.example/` rather than
     * `example.com/`).
     * - Any prefix where the second label is `modelcontextprotocol` or `mcp` is **reserved**
     * for MCP use. For example: `io.modelcontextprotocol/`, `dev.mcp/`,
     * `org.modelcontextprotocol.api/`, and `com.mcp.tools/` are all reserved. However,
     * `com.example.mcp/` is NOT reserved, as the second label is `example`.
     *
     * **Name:**
     * - Unless empty, MUST start and end with an alphanumeric character (`[a-z0-9A-Z]`).
     * - Interior characters may be alphanumeric, hyphens (`-`), underscores (`_`), or dots
     * (`.`).
     *
     * Optional metadata about the tool use. Clients SHOULD preserve this field when
     * including tool uses in subsequent sampling requests to enable caching optimizations.
     *
     * Optional metadata about the tool result. Clients SHOULD preserve this field when
     * including tool results in subsequent sampling requests to enable caching optimizations.
     */
    MetaObject _meta;
    /**
     * Optional annotations for the client.
     */
    Annotations annotations;
    /**
     * The text content of the message.
     */
    std::string text;
    SamplingMessageContentBlock_type type;
    /**
     * The base64-encoded image data.
     *
     * The base64-encoded audio data.
     */
    std::string data;
    /**
     * The MIME type of the image. Different providers may support different image types.
     *
     * The MIME type of the audio. Different providers may support different audio types.
     */
    std::string mimeType;
    /**
     * A unique identifier for this tool use.
     *
     * This ID is used to match tool results to their corresponding tool uses.
     */
    std::string id;
    /**
     * The arguments to pass to the tool, conforming to the tool's input schema.
     */
    std::map<std::string, JsonValue> input;
    /**
     * The name of the tool to call.
     */
    std::string name;
    /**
     * The unstructured result content of the tool use.
     *
     * This has the same format as {@link CallToolResult.content} and can include text, images,
     * audio, resource links, and embedded resources.
     */
    std::vector<ContentBlock> content;
    /**
     * Whether the tool use resulted in an error.
     *
     * If true, the content typically describes the error that occurred.
     * Default: false
     */
    bool isError;
    /**
     * An optional structured result value.
     *
     * This can be any JSON value (object, array, string, number, boolean, or null).
     * If the tool defined an {@link Tool.outputSchema}, this SHOULD conform to that schema.
     */
    JsonValue structuredContent;
    /**
     * The ID of the tool use this result corresponds to.
     *
     * This MUST match the ID from a previous {@link ToolUseContent}.
     */
    std::string toolUseId;
};

/**
 * Text provided to or from an LLM.
 *
 * An image provided to or from an LLM.
 *
 * Audio provided to or from an LLM.
 *
 * A request from the assistant to call a tool.
 *
 * The result of a tool use, provided by the user back to the assistant.
 */
struct _content final {
    /**
     * Represents the contents of a `_meta` field, which clients and servers use to attach
     * additional metadata to their interactions.
     *
     * Certain key names are reserved by MCP for protocol-level metadata; implementations MUST
     * NOT make assumptions about values at these keys. Additionally, specific schema
     * definitions may reserve particular names for purpose-specific metadata, as declared in
     * those definitions.
     *
     * Valid keys have two segments:
     *
     * **Prefix:**
     * - Optional 
 if specified, MUST be a series of _labels_ separated by dots (`.`), followed
     * by a slash (`/`).
     * - Labels MUST start with a letter and end with a letter or digit. Interior characters may
     * be letters, digits, or hyphens (`-`).
     * - Implementations SHOULD use reverse DNS notation (e.g., `com.example/` rather than
     * `example.com/`).
     * - Any prefix where the second label is `modelcontextprotocol` or `mcp` is **reserved**
     * for MCP use. For example: `io.modelcontextprotocol/`, `dev.mcp/`,
     * `org.modelcontextprotocol.api/`, and `com.mcp.tools/` are all reserved. However,
     * `com.example.mcp/` is NOT reserved, as the second label is `example`.
     *
     * **Name:**
     * - Unless empty, MUST start and end with an alphanumeric character (`[a-z0-9A-Z]`).
     * - Interior characters may be alphanumeric, hyphens (`-`), underscores (`_`), or dots
     * (`.`).
     *
     * Optional metadata about the tool use. Clients SHOULD preserve this field when
     * including tool uses in subsequent sampling requests to enable caching optimizations.
     *
     * Optional metadata about the tool result. Clients SHOULD preserve this field when
     * including tool results in subsequent sampling requests to enable caching optimizations.
     */
    MetaObject _meta;
    /**
     * Optional annotations for the client.
     */
    Annotations annotations;
    /**
     * The text content of the message.
     */
    std::string text;
    SamplingMessageContentBlock_type type;
    /**
     * The base64-encoded image data.
     *
     * The base64-encoded audio data.
     */
    std::string data;
    /**
     * The MIME type of the image. Different providers may support different image types.
     *
     * The MIME type of the audio. Different providers may support different audio types.
     */
    std::string mimeType;
    /**
     * A unique identifier for this tool use.
     *
     * This ID is used to match tool results to their corresponding tool uses.
     */
    std::string id;
    /**
     * The arguments to pass to the tool, conforming to the tool's input schema.
     */
    std::map<std::string, JsonValue> input;
    /**
     * The name of the tool to call.
     */
    std::string name;
    /**
     * The unstructured result content of the tool use.
     *
     * This has the same format as {@link CallToolResult.content} and can include text, images,
     * audio, resource links, and embedded resources.
     */
    std::vector<ContentBlock> content;
    /**
     * Whether the tool use resulted in an error.
     *
     * If true, the content typically describes the error that occurred.
     * Default: false
     */
    bool isError;
    /**
     * An optional structured result value.
     *
     * This can be any JSON value (object, array, string, number, boolean, or null).
     * If the tool defined an {@link Tool.outputSchema}, this SHOULD conform to that schema.
     */
    JsonValue structuredContent;
    /**
     * The ID of the tool use this result corresponds to.
     *
     * This MUST match the ID from a previous {@link ToolUseContent}.
     */
    std::string toolUseId;
};

using SamplingMessage_content = std::variant<std::vector<SamplingMessageContentBlock>, _content>;

/**
 * Describes a message issued to or received from an LLM API.
 */
struct SamplingMessage final {
    /**
     * Represents the contents of a `_meta` field, which clients and servers use to attach
     * additional metadata to their interactions.
     *
     * Certain key names are reserved by MCP for protocol-level metadata; implementations MUST
     * NOT make assumptions about values at these keys. Additionally, specific schema
     * definitions may reserve particular names for purpose-specific metadata, as declared in
     * those definitions.
     *
     * Valid keys have two segments:
     *
     * **Prefix:**
     * - Optional 
 if specified, MUST be a series of _labels_ separated by dots (`.`), followed
     * by a slash (`/`).
     * - Labels MUST start with a letter and end with a letter or digit. Interior characters may
     * be letters, digits, or hyphens (`-`).
     * - Implementations SHOULD use reverse DNS notation (e.g., `com.example/` rather than
     * `example.com/`).
     * - Any prefix where the second label is `modelcontextprotocol` or `mcp` is **reserved**
     * for MCP use. For example: `io.modelcontextprotocol/`, `dev.mcp/`,
     * `org.modelcontextprotocol.api/`, and `com.mcp.tools/` are all reserved. However,
     * `com.example.mcp/` is NOT reserved, as the second label is `example`.
     *
     * **Name:**
     * - Unless empty, MUST start and end with an alphanumeric character (`[a-z0-9A-Z]`).
     * - Interior characters may be alphanumeric, hyphens (`-`), underscores (`_`), or dots
     * (`.`).
     */
    MetaObject _meta;
    SamplingMessage_content content;
    Role role;
};

enum class params_mode : int { form, url };

/**
 * Hints to use for model selection.
 *
 * Keys not declared here are currently left unspecified by the spec and are up
 * to the client to interpret.
 */
struct ModelHint final {
    /**
     * A hint for a model name.
     *
     * The client SHOULD treat this as a substring of a model name; for example:
     * - `claude-3-5-sonnet` should match `claude-3-5-sonnet-20241022`
     * - `sonnet` should match `claude-3-5-sonnet-20241022`, `claude-3-sonnet-20240229`, etc.
     * - `claude` should match any Claude model
     *
     * The client MAY also map the string to a different provider's model name or a different
     * model family, as long as it fills a similar niche; for example:
     * - `gemini-1.5-flash` could match `claude-3-haiku-20240307`
     */
    std::string name;
};

/**
 * The server's preferences for which model to select. The client MAY ignore these
 * preferences.
 *
 * The server's preferences for model selection, requested of the client during sampling.
 *
 * Because LLMs can vary along multiple dimensions, choosing the "best" model is
 * rarely straightforward.  Different models excel in different areas
some are
 * faster but less capable, others are more capable but more expensive, and so
 * on. This interface allows servers to express their priorities across multiple
 * dimensions to help clients make an appropriate selection for their use case.
 *
 * These preferences are always advisory. The client MAY ignore them. It is also
 * up to the client to decide how to interpret these preferences and how to
 * balance them against other considerations.
 */
struct ModelPreferences final {
    /**
     * How much to prioritize cost when selecting a model. A value of 0 means cost
     * is not important, while a value of 1 means cost is the most important
     * factor.
     */
    double costPriority;
    /**
     * Optional hints to use for model selection.
     *
     * If multiple hints are specified, the client MUST evaluate them in order
     * (such that the first match is taken).
     *
     * The client SHOULD prioritize these hints over the numeric priorities, but
     * MAY still use the priorities to select from ambiguous matches.
     */
    std::vector<ModelHint> hints;
    /**
     * How much to prioritize intelligence and capabilities when selecting a
     * model. A value of 0 means intelligence is not important, while a value of 1
     * means intelligence is the most important factor.
     */
    double intelligencePriority;
    /**
     * How much to prioritize sampling speed (latency) when selecting a model. A
     * value of 0 means speed is not important, while a value of 1 means speed is
     * the most important factor.
     */
    double speedPriority;
};

using default_value = std::variant<std::vector<std::string>, bool, double, std::string>;

enum class format : int { date, datetime, email, uri };

struct anyOf final {
    /**
     * The constant enum value.
     */
    std::string anyOf_const;
    /**
     * Display title for this option.
     */
    std::string title;
};

enum class items_type : int { string };

/**
 * Schema for the array items.
 *
 * Schema for array items with enum options and display labels.
 */
struct PrimitiveSchemaDefinition_items final {
    /**
     * Array of enum values to choose from.
     */
    std::vector<std::string> items_enum;
    items_type type;
    /**
     * Array of enum options with values and display labels.
     */
    std::vector<anyOf> anyOf;
};

struct oneOf final {
    /**
     * The enum value.
     */
    std::string oneOf_const;
    /**
     * Display label for this option.
     */
    std::string title;
};

enum class PrimitiveSchemaDefinition_type : int { array, boolean, integer, number, string };

/**
 * Restricted schema definitions that only allow primitive types
 * without nested objects or arrays.
 *
 * Schema for single-selection enumeration without display titles for options.
 *
 * Schema for single-selection enumeration with display titles for each option.
 *
 * Schema for multiple-selection enumeration without display titles for options.
 *
 * Schema for multiple-selection enumeration with display titles for each option.
 *
 * Use {@link TitledSingleSelectEnumSchema} instead.
 * This interface will be removed in a future version.
 */
struct PrimitiveSchemaDefinition final {
    /**
     * Optional default value.
     */
    default_value PrimitiveSchemaDefinition_default;
    /**
     * Optional description for the enum field.
     */
    std::string description;
    format format;
    double maxLength;
    double minLength;
    /**
     * Optional title for the enum field.
     */
    std::string title;
    PrimitiveSchemaDefinition_type type;
    double maximum;
    double minimum;
    /**
     * Array of enum values to choose from.
     */
    std::vector<std::string> PrimitiveSchemaDefinition_enum;
    /**
     * Array of enum options with values and display labels.
     */
    std::vector<oneOf> oneOf;
    /**
     * Schema for the array items.
     *
     * Schema for array items with enum options and display labels.
     */
    PrimitiveSchemaDefinition_items items;
    /**
     * Maximum number of items to select.
     */
    double maxItems;
    /**
     * Minimum number of items to select.
     */
    double minItems;
    /**
     * (Legacy) Display names for enum values.
     * Non-standard according to JSON schema 2020-12.
     */
    std::vector<std::string> enumNames;
};

enum class requestedSchema_type : int { object };

/**
 * A restricted subset of JSON Schema.
 * Only top-level properties are allowed, without nesting.
 */
struct requestedSchema final {
    std::string schema;
    std::map<std::string, PrimitiveSchemaDefinition> properties;
    std::vector<std::string> required;
    requestedSchema_type type;
};

/**
 * Controls the tool use ability of the model:
 * - `"auto"`: Model decides whether to use tools (default)
 * - `"required"`: Model MUST use at least one tool before completing
 * - `"none"`: Model MUST NOT use any tools
 */
enum class ToolChoice_mode : int { mode_auto, none, required };

/**
 * Controls how the model uses tools.
 * The client MUST return an error if this field is provided but {@link
 * ClientCapabilities.sampling.tools} is not declared.
 * Default is `{ mode: "auto" }`.
 *
 * Controls tool selection behavior for sampling requests.
 */
struct ToolChoice final {
    /**
     * Controls the tool use ability of the model:
     * - `"auto"`: Model decides whether to use tools (default)
     * - `"required"`: Model MUST use at least one tool before completing
     * - `"none"`: Model MUST NOT use any tools
     */
    ToolChoice_mode mode;
};

/**
 * Optional additional tool information.
 *
 * Display name precedence order is: `title`, `annotations.title`, then `name`.
 *
 * Additional properties describing a {@link Tool} to clients.
 *
 * NOTE: all properties in `ToolAnnotations` are **hints**.
 * They are not guaranteed to provide a faithful description of
 * tool behavior (including descriptive properties like `title`).
 *
 * Clients should never make tool use decisions based on `ToolAnnotations`
 * received from untrusted servers.
 */
struct ToolAnnotations final {
    /**
     * If true, the tool may perform destructive updates to its environment.
     * If false, the tool performs only additive updates.
     *
     * (This property is meaningful only when `readOnlyHint == false`)
     *
     * Default: true
     */
    bool destructiveHint;
    /**
     * If true, calling the tool repeatedly with the same arguments
     * will have no additional effect on its environment.
     *
     * (This property is meaningful only when `readOnlyHint == false`)
     *
     * Default: false
     */
    bool idempotentHint;
    /**
     * If true, this tool may interact with an "open world" of external
     * entities. If false, the tool's domain of interaction is closed.
     * For example, the world of a web search tool is open, whereas that
     * of a memory tool is not.
     *
     * Default: true
     */
    bool openWorldHint;
    /**
     * If true, the tool does not modify its environment.
     *
     * Default: false
     */
    bool readOnlyHint;
    /**
     * A human-readable title for the tool.
     */
    std::string title;
};

/**
 * A JSON Schema object defining the expected parameters for the tool.
 *
 * Tool arguments are always JSON objects, so `type: "object"` is required at the root.
 * Beyond that, any JSON Schema 2020-12 keyword may appear alongside `type` 
 including
 * composition keywords (`oneOf`, `anyOf`, `allOf`, `not`), conditional keywords
 * (`if`/`then`/`else`), reference keywords (`$ref`, `$defs`, `$anchor`), and any other
 * standard validation or annotation keywords.
 *
 * Property schemas may carry an `x-mcp-header` annotation to mirror the
 * argument value into an HTTP header on the Streamable HTTP transport. See
 * the Streamable HTTP transport specification for the validity and
 * extraction rules.
 *
 * Defaults to JSON Schema 2020-12 when no explicit `$schema` is provided.
 */
struct inputSchema final {
    std::string schema;
    requestedSchema_type type;
};

/**
 * An optional JSON Schema object defining the structure of the tool's output returned in
 * the structuredContent field of a {@link CallToolResult}. This can be any valid JSON
 * Schema 2020-12.
 *
 * Defaults to JSON Schema 2020-12 when no explicit `$schema` is provided.
 */
struct outputSchema final {
    std::string schema;
};

/**
 * Definition for a tool the client can call.
 */
struct Tool final {
    /**
     * Represents the contents of a `_meta` field, which clients and servers use to attach
     * additional metadata to their interactions.
     *
     * Certain key names are reserved by MCP for protocol-level metadata; implementations MUST
     * NOT make assumptions about values at these keys. Additionally, specific schema
     * definitions may reserve particular names for purpose-specific metadata, as declared in
     * those definitions.
     *
     * Valid keys have two segments:
     *
     * **Prefix:**
     * - Optional 
 if specified, MUST be a series of _labels_ separated by dots (`.`), followed
     * by a slash (`/`).
     * - Labels MUST start with a letter and end with a letter or digit. Interior characters may
     * be letters, digits, or hyphens (`-`).
     * - Implementations SHOULD use reverse DNS notation (e.g., `com.example/` rather than
     * `example.com/`).
     * - Any prefix where the second label is `modelcontextprotocol` or `mcp` is **reserved**
     * for MCP use. For example: `io.modelcontextprotocol/`, `dev.mcp/`,
     * `org.modelcontextprotocol.api/`, and `com.mcp.tools/` are all reserved. However,
     * `com.example.mcp/` is NOT reserved, as the second label is `example`.
     *
     * **Name:**
     * - Unless empty, MUST start and end with an alphanumeric character (`[a-z0-9A-Z]`).
     * - Interior characters may be alphanumeric, hyphens (`-`), underscores (`_`), or dots
     * (`.`).
     */
    MetaObject _meta;
    /**
     * Optional additional tool information.
     *
     * Display name precedence order is: `title`, `annotations.title`, then `name`.
     */
    ToolAnnotations annotations;
    /**
     * A human-readable description of the tool.
     *
     * This can be used by clients to improve the LLM's understanding of available tools. It can
     * be thought of like a "hint" to the model.
     */
    std::string description;
    /**
     * Optional set of sized icons that the client can display in a user interface.
     *
     * Clients that support rendering icons MUST support at least the following MIME types:
     * - `image/png` - PNG images (safe, universal compatibility)
     * - `image/jpeg` (and `image/jpg`) - JPEG images (safe, universal compatibility)
     *
     * Clients that support rendering icons SHOULD also support:
     * - `image/svg+xml` - SVG images (scalable but requires security precautions)
     * - `image/webp` - WebP images (modern, efficient format)
     */
    std::vector<Icon> icons;
    /**
     * A JSON Schema object defining the expected parameters for the tool.
     *
     * Tool arguments are always JSON objects, so `type: "object"` is required at the root.
     * Beyond that, any JSON Schema 2020-12 keyword may appear alongside `type` 
 including
     * composition keywords (`oneOf`, `anyOf`, `allOf`, `not`), conditional keywords
     * (`if`/`then`/`else`), reference keywords (`$ref`, `$defs`, `$anchor`), and any other
     * standard validation or annotation keywords.
     *
     * Property schemas may carry an `x-mcp-header` annotation to mirror the
     * argument value into an HTTP header on the Streamable HTTP transport. See
     * the Streamable HTTP transport specification for the validity and
     * extraction rules.
     *
     * Defaults to JSON Schema 2020-12 when no explicit `$schema` is provided.
     */
    inputSchema inputSchema;
    /**
     * Intended for programmatic or logical use, but used as a display name in past specs or
     * fallback (if title isn't present).
     */
    std::string name;
    /**
     * An optional JSON Schema object defining the structure of the tool's output returned in
     * the structuredContent field of a {@link CallToolResult}. This can be any valid JSON
     * Schema 2020-12.
     *
     * Defaults to JSON Schema 2020-12 when no explicit `$schema` is provided.
     */
    outputSchema outputSchema;
    /**
     * Intended for UI and end-user contexts 
 optimized to be human-readable and easily
     * understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for {@link Tool},
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::string title;
};

/**
 * Parameters for a `sampling/createMessage` request.
 *
 * The parameters for a request to elicit non-sensitive information from the user via a form
 * in the client.
 *
 * The parameters for a request to elicit information from the user via a URL in the client.
 */
struct InputRequest_params final {
    /**
     * A request to include context from one or more MCP servers (including the caller), to be
     * attached to the prompt.
     * The client MAY ignore this request.
     *
     * Default is `"none"`. The values `"thisServer"` and `"allServers"` are deprecated
     * (SEP-2596): servers SHOULD
     * omit this field or use `"none"`, and SHOULD only use the deprecated values if the client
     * declares
     * {@link ClientCapabilities.sampling.context}.
     */
    includeContext includeContext;
    /**
     * The requested maximum number of tokens to sample (to prevent runaway completions).
     *
     * The client MAY choose to sample fewer tokens than the requested maximum.
     */
    double maxTokens;
    std::vector<SamplingMessage> messages;
    /**
     * Optional metadata to pass through to the LLM provider. The format of this metadata is
     * provider-specific.
     */
    std::map<std::string, JsonValue> metadata;
    /**
     * The server's preferences for which model to select. The client MAY ignore these
     * preferences.
     */
    ModelPreferences modelPreferences;
    std::vector<std::string> stopSequences;
    /**
     * An optional system prompt the server wants to use for sampling. The client MAY modify or
     * omit this prompt.
     */
    std::string systemPrompt;
    double temperature;
    /**
     * Controls how the model uses tools.
     * The client MUST return an error if this field is provided but {@link
     * ClientCapabilities.sampling.tools} is not declared.
     * Default is `{ mode: "auto" }`.
     */
    ToolChoice toolChoice;
    /**
     * Tools that the model may use during generation.
     * The client MUST return an error if this field is provided but {@link
     * ClientCapabilities.sampling.tools} is not declared.
     */
    std::vector<Tool> tools;
    /**
     * Represents the contents of a `_meta` field, which clients and servers use to attach
     * additional metadata to their interactions.
     *
     * Certain key names are reserved by MCP for protocol-level metadata; implementations MUST
     * NOT make assumptions about values at these keys. Additionally, specific schema
     * definitions may reserve particular names for purpose-specific metadata, as declared in
     * those definitions.
     *
     * Valid keys have two segments:
     *
     * **Prefix:**
     * - Optional 
 if specified, MUST be a series of _labels_ separated by dots (`.`), followed
     * by a slash (`/`).
     * - Labels MUST start with a letter and end with a letter or digit. Interior characters may
     * be letters, digits, or hyphens (`-`).
     * - Implementations SHOULD use reverse DNS notation (e.g., `com.example/` rather than
     * `example.com/`).
     * - Any prefix where the second label is `modelcontextprotocol` or `mcp` is **reserved**
     * for MCP use. For example: `io.modelcontextprotocol/`, `dev.mcp/`,
     * `org.modelcontextprotocol.api/`, and `com.mcp.tools/` are all reserved. However,
     * `com.example.mcp/` is NOT reserved, as the second label is `example`.
     *
     * **Name:**
     * - Unless empty, MUST start and end with an alphanumeric character (`[a-z0-9A-Z]`).
     * - Interior characters may be alphanumeric, hyphens (`-`), underscores (`_`), or dots
     * (`.`).
     */
    MetaObject _meta;
    /**
     * The message to present to the user describing what information is being requested.
     *
     * The message to present to the user explaining why the interaction is needed.
     */
    std::string message;
    /**
     * The elicitation mode.
     */
    params_mode mode;
    /**
     * A restricted subset of JSON Schema.
     * Only top-level properties are allowed, without nesting.
     */
    requestedSchema requestedSchema;
    /**
     * The URL that the user should navigate to.
     */
    std::string url;
};

/**
 * A request from the server to sample an LLM via the client. The client has full discretion
 * over which model to select. The client should also inform the user before beginning
 * sampling, to allow them to inspect the request (human in the loop) and decide whether to
 * approve it.
 *
 * Sent from the server to request a list of root URIs from the client. Roots allow
 * servers to ask for specific directories or files to operate on. A common example
 * for roots is providing a set of repositories or directories a server should operate
 * on.
 *
 * This request is typically used when the server needs to understand the file system
 * structure or access specific locations that the client has permission to read from.
 *
 * A request from the server to elicit additional information from the user via the client.
 */
struct InputRequest final {
    InputRequest_method method;
    InputRequest_params params;
};

/**
 * An InputRequiredResult sent by the server to indicate that additional input is needed
 * before the request can be completed.
 *
 * At least one of `inputRequests` or `requestState` MUST be present.
 */
struct InputRequiredResult final {
    /**
     * Extends {@link MetaObject} with additional result-specific fields. All key naming rules
     * from `MetaObject` apply.
     */
    ResultMetaObject _meta;
    /**
     * A map of server-initiated requests that the client must fulfill.
     * Keys are server-assigned identifiers; values are the request objects.
     */
    std::map<std::string, InputRequest> inputRequests;
    std::string requestState;
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    std::string resultType;
};

/**
 * The desired log level for this request. Optional.
 *
 * If absent, the server MUST NOT send any {@link
 * LoggingMessageNotificationnotifications/message}
 * notifications for this request. The client opts in to log messages by
 * explicitly setting a level. Replaces the former `logging/setLevel` RPC.
 *
 * The severity of this log message.
 *
 * The severity of a log message.
 *
 * These map to syslog message severities, as specified in RFC-5424:
 * https://datatracker.ietf.org/doc/html/rfc5424#section-6.2.1
 */
enum class LoggingLevel : int { alert, critical, debug, emergency, error, info, notice, warning };

/**
 * Extends {@link MetaObject} with additional request-specific fields. All key naming rules
 * from `MetaObject` apply.
 */
struct RequestMetaObject final {
    /**
     * The client's capabilities for this specific request. Required.
     *
     * Capabilities are declared per-request rather than once at initialization;
     * an empty object means the client supports no optional capabilities.
     * Servers MUST NOT infer capabilities from prior requests.
     */
    ClientCapabilities iomodelcontextprotocolclientCapabilities;
    /**
     * Identifies the client software making the request. Clients SHOULD
     * include this field on every request unless specifically configured not
     * to do so.
     *
     * The {@link Implementation} schema requires `name` and `version`; other
     * fields are optional.
     *
     * The value is self-reported by the client and is not verified by the
     * protocol. It is intended for display, logging, and debugging. Servers
     * SHOULD NOT use it to change their behavior, and SHOULD NOT rely on it for
     * security decisions.
     */
    Implementation iomodelcontextprotocolclientInfo;
    /**
     * The desired log level for this request. Optional.
     *
     * If absent, the server MUST NOT send any {@link
     * LoggingMessageNotificationnotifications/message}
     * notifications for this request. The client opts in to log messages by
     * explicitly setting a level. Replaces the former `logging/setLevel` RPC.
     */
    LoggingLevel iomodelcontextprotocollogLevel;
    /**
     * The MCP Protocol Version being used for this request. Required.
     *
     * For the HTTP transport, this value MUST match the `MCP-Protocol-Version`
     * header; otherwise the server MUST return a `400 Bad Request`. If the
     * server does not support the requested version, it MUST return an
     * {@link UnsupportedProtocolVersionError}.
     */
    std::string iomodelcontextprotocolprotocolVersion;
    /**
     * If specified, the caller is requesting out-of-band progress notifications for this
     * request (as represented by {@link ProgressNotificationnotifications/progress}). The value
     * of this parameter is an opaque token that will be attached to any subsequent
     * notifications. The receiver is not obligated to provide these notifications.
     */
    RequestId progressToken;
};

/**
 * The user action in response to the elicitation.
 * - `"accept"`: User submitted the form/confirmed the action
 * - `"decline"`: User explicitly declined the action
 * - `"cancel"`: User dismissed without making an explicit choice
 */
enum class action : int { accept, cancel, decline };

using InputResponse_content = std::variant<std::vector<SamplingMessageContentBlock>, std::map<std::string, JsonValue>>;

/**
 * Represents a root directory or file that the server can operate on.
 */
struct Root final {
    /**
     * Represents the contents of a `_meta` field, which clients and servers use to attach
     * additional metadata to their interactions.
     *
     * Certain key names are reserved by MCP for protocol-level metadata; implementations MUST
     * NOT make assumptions about values at these keys. Additionally, specific schema
     * definitions may reserve particular names for purpose-specific metadata, as declared in
     * those definitions.
     *
     * Valid keys have two segments:
     *
     * **Prefix:**
     * - Optional 
 if specified, MUST be a series of _labels_ separated by dots (`.`), followed
     * by a slash (`/`).
     * - Labels MUST start with a letter and end with a letter or digit. Interior characters may
     * be letters, digits, or hyphens (`-`).
     * - Implementations SHOULD use reverse DNS notation (e.g., `com.example/` rather than
     * `example.com/`).
     * - Any prefix where the second label is `modelcontextprotocol` or `mcp` is **reserved**
     * for MCP use. For example: `io.modelcontextprotocol/`, `dev.mcp/`,
     * `org.modelcontextprotocol.api/`, and `com.mcp.tools/` are all reserved. However,
     * `com.example.mcp/` is NOT reserved, as the second label is `example`.
     *
     * **Name:**
     * - Unless empty, MUST start and end with an alphanumeric character (`[a-z0-9A-Z]`).
     * - Interior characters may be alphanumeric, hyphens (`-`), underscores (`_`), or dots
     * (`.`).
     */
    MetaObject _meta;
    /**
     * An optional name for the root. This can be used to provide a human-readable
     * identifier for the root, which may be useful for display purposes or for
     * referencing the root in other parts of the application.
     */
    std::string name;
    /**
     * The URI identifying the root. This *must* start with `file://` for now.
     * This restriction may be relaxed in future versions of the protocol to allow
     * other URI schemes.
     */
    std::string uri;
};

/**
 * The result returned by the client for a {@link
 * CreateMessageRequestsampling/createMessage} request.
 * The client should inform the user before returning the sampled message, to allow them
 * to inspect the response (human in the loop) and decide whether to allow the server to see
 * it.
 *
 * The result returned by the client for a {@link ListRootsRequestroots/list} request.
 * This result contains an array of {@link Root} objects, each representing a root directory
 * or file that the server can operate on.
 *
 * The result returned by the client for an {@link ElicitRequestelicitation/create} request.
 */
struct InputResponse final {
    /**
     * Represents the contents of a `_meta` field, which clients and servers use to attach
     * additional metadata to their interactions.
     *
     * Certain key names are reserved by MCP for protocol-level metadata; implementations MUST
     * NOT make assumptions about values at these keys. Additionally, specific schema
     * definitions may reserve particular names for purpose-specific metadata, as declared in
     * those definitions.
     *
     * Valid keys have two segments:
     *
     * **Prefix:**
     * - Optional 
 if specified, MUST be a series of _labels_ separated by dots (`.`), followed
     * by a slash (`/`).
     * - Labels MUST start with a letter and end with a letter or digit. Interior characters may
     * be letters, digits, or hyphens (`-`).
     * - Implementations SHOULD use reverse DNS notation (e.g., `com.example/` rather than
     * `example.com/`).
     * - Any prefix where the second label is `modelcontextprotocol` or `mcp` is **reserved**
     * for MCP use. For example: `io.modelcontextprotocol/`, `dev.mcp/`,
     * `org.modelcontextprotocol.api/`, and `com.mcp.tools/` are all reserved. However,
     * `com.example.mcp/` is NOT reserved, as the second label is `example`.
     *
     * **Name:**
     * - Unless empty, MUST start and end with an alphanumeric character (`[a-z0-9A-Z]`).
     * - Interior characters may be alphanumeric, hyphens (`-`), underscores (`_`), or dots
     * (`.`).
     */
    MetaObject _meta;
    /**
     * The submitted form data, only present when action is `"accept"` and mode was `"form"`.
     * Contains values matching the requested schema.
     * Omitted for out-of-band mode responses.
     */
    InputResponse_content content;
    /**
     * The name of the model that generated the message.
     */
    std::string model;
    Role role;
    /**
     * The reason why sampling stopped, if known.
     *
     * Standard values:
     * - `"endTurn"`: Natural end of the assistant's turn
     * - `"stopSequence"`: A stop sequence was encountered
     * - `"maxTokens"`: Maximum token limit was reached
     * - `"toolUse"`: The model wants to use one or more tools
     *
     * This field is an open string to allow for provider-specific stop reasons.
     */
    std::string stopReason;
    std::vector<Root> roots;
    /**
     * The user action in response to the elicitation.
     * - `"accept"`: User submitted the form/confirmed the action
     * - `"decline"`: User explicitly declined the action
     * - `"cancel"`: User dismissed without making an explicit choice
     */
    action action;
};

struct InputResponseRequestParams final {
    RequestMetaObject _meta;
    /**
     * A map of client responses to server-initiated requests.
     * Keys correspond to the keys in the {@link InputRequests} map;
     * values are the client's result for each request.
     */
    std::map<std::string, InputResponse> inputResponses;
    std::string requestState;
};

enum class CancelledNotification_method : int { notificationscancelled };

/**
 * Extends {@link MetaObject} with additional notification-specific fields. All key naming
 * rules from `MetaObject` apply.
 */
struct NotificationMetaObject final {
    /**
     * Identifies the subscription stream a notification was delivered on. The
     * server MUST include this key on every notification delivered via a
     * {@link SubscriptionsListenRequestsubscriptions/listen} stream, so the
     * client can correlate the notification with the originating subscription.
     * The key is absent on notifications not delivered via a subscription
     * stream (e.g. progress notifications for an in-flight request), which is
     * why it is optional here.
     *
     * The value is the JSON-RPC ID of the `subscriptions/listen` request that
     * opened the stream.
     */
    RequestId iomodelcontextprotocolsubscriptionId;
};

/**
 * Parameters for a `notifications/cancelled` notification.
 */
struct CancelledNotificationParams final {
    /**
     * Extends {@link MetaObject} with additional notification-specific fields. All key naming
     * rules from `MetaObject` apply.
     */
    NotificationMetaObject _meta;
    /**
     * An optional string describing the reason for the cancellation. This MAY be logged or
     * presented to the user.
     */
    std::string reason;
    /**
     * The ID of the request to cancel.
     *
     * This MUST correspond to the ID of a request the client previously issued.
     */
    RequestId requestId;
};

/**
 * This notification is sent by the client to indicate that it is cancelling a request it
 * previously issued.
 *
 * On stdio, the server also sends this notification, solely to terminate a {@link
 * SubscriptionsListenRequestsubscriptions/listen} stream: it references the ID of the
 * `subscriptions/listen` request that opened the stream. Servers MUST NOT use this
 * notification to cancel any other request.
 *
 * The request SHOULD still be in-flight, but due to communication latency, it is always
 * possible that this notification MAY arrive after the request has already finished.
 *
 * This notification indicates that the result will be unused, so any associated processing
 * SHOULD cease.
 */
struct ClientNotification final {
    jsonrpc jsonrpc;
    CancelledNotification_method method;
    CancelledNotificationParams params;
};

enum class DiscoverRequest_method : int { serverdiscover };

/**
 * Common params for any request.
 */
struct RequestParams final {
    RequestMetaObject _meta;
};

/**
 * A request from the client asking the server to advertise its supported
 * protocol versions, capabilities, and other metadata. Servers **MUST**
 * implement `server/discover`. Clients **MAY** call it but are not required
 * to 
 version negotiation can also happen inline via per-request `_meta`.
 */
struct DiscoverRequest final {
    RequestId id;
    jsonrpc jsonrpc;
    DiscoverRequest_method method;
    RequestParams params;
};

/**
 * Indicates the intended scope of the cached response, analogous to HTTP
 * `Cache-Control: public` vs `Cache-Control: private`.
 *
 * - `"public"`: The response does not contain user-specific data. Any
 * client or intermediary (e.g., shared gateway, caching proxy) MAY cache
 * the response and serve it across authorization contexts.
 * - `"private"`: The response MAY be cached and reused only within the
 * same authorization context. Caches MUST NOT be shared across
 * authorization contexts (e.g., a different access token requires a
 * different cache).
 */
enum class cacheScope : int { cacheScope_private, cacheScope_public };

/**
 * Present if the server offers any prompt templates.
 */
struct prompts final {
    /**
     * Whether this server supports notifications for changes to the prompt list.
     */
    bool listChanged;
};

/**
 * Present if the server offers any resources to read.
 */
struct resources final {
    /**
     * Whether this server supports notifications for changes to the resource list.
     */
    bool listChanged;
    /**
     * Whether this server supports subscribing to resource updates.
     */
    bool subscribe;
};

/**
 * Present if the server offers any tools to call.
 */
struct tools final {
    /**
     * Whether this server supports notifications for changes to the tool list.
     */
    bool listChanged;
};

/**
 * The capabilities of the server.
 *
 * Capabilities that a server may support. Known capabilities are defined here, in this
 * schema, but this is not a closed set: any server can define its own, additional
 * capabilities.
 */
struct ServerCapabilities final {
    /**
     * Present if the server supports argument autocompletion suggestions.
     */
    std::map<std::string, JsonValue> completions;
    /**
     * Experimental, non-standard capabilities that the server supports.
     */
    std::map<std::string, std::map<std::string, JsonValue>> experimental;
    /**
     * Optional MCP extensions that the server supports. Keys are extension identifiers
     * (e.g., "io.modelcontextprotocol/tasks"), and values are per-extension settings
     * objects. An empty object indicates support with no settings.
     *
     * Keys MUST follow the {@link MetaObject`_meta` key naming rules}, with a
     * mandatory prefix.
     */
    std::map<std::string, std::map<std::string, JsonValue>> extensions;
    /**
     * Present if the server supports sending log messages to the client.
     */
    std::map<std::string, JsonValue> logging;
    /**
     * Present if the server offers any prompt templates.
     */
    prompts prompts;
    /**
     * Present if the server offers any resources to read.
     */
    resources resources;
    /**
     * Present if the server offers any tools to call.
     */
    tools tools;
};

/**
 * The result returned by the server for a {@link DiscoverRequestserver/discover} request.
 */
struct DiscoverResult final {
    /**
     * Extends {@link MetaObject} with additional result-specific fields. All key naming rules
     * from `MetaObject` apply.
     */
    ResultMetaObject _meta;
    /**
     * Indicates the intended scope of the cached response, analogous to HTTP
     * `Cache-Control: public` vs `Cache-Control: private`.
     *
     * - `"public"`: The response does not contain user-specific data. Any
     * client or intermediary (e.g., shared gateway, caching proxy) MAY cache
     * the response and serve it across authorization contexts.
     * - `"private"`: The response MAY be cached and reused only within the
     * same authorization context. Caches MUST NOT be shared across
     * authorization contexts (e.g., a different access token requires a
     * different cache).
     */
    cacheScope cacheScope;
    /**
     * The capabilities of the server.
     */
    ServerCapabilities capabilities;
    /**
     * Natural-language guidance describing the server and its features.
     *
     * This can be used by clients to improve an LLM's understanding of
     * available tools (e.g., by including it in a system prompt). It should
     * focus on information that helps the model use the server effectively
     * and should not duplicate information already in tool descriptions.
     */
    std::string instructions;
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    std::string resultType;
    /**
     * MCP Protocol Versions this server supports. The client should choose a
     * version from this list for use in subsequent requests.
     */
    std::vector<std::string> supportedVersions;
    /**
     * A hint from the server indicating how long (in milliseconds) the
     * client MAY cache this response before re-fetching. Semantics are
     * analogous to HTTP Cache-Control max-age.
     *
     * - If 0, The response SHOULD be considered immediately stale,
     * The client MAY re-fetch every time the result is needed.
     * - If positive, the client SHOULD consider the result fresh for this many
     * milliseconds after receiving the response.
     */
    double ttlMs;
};

/**
 * A successful response from the server for a {@link DiscoverRequestserver/discover}
 * request.
 */
struct DiscoverResultResponse final {
    RequestId id;
    jsonrpc jsonrpc;
    DiscoverResult result;
};

/**
 * Base interface to add `icons` property.
 */
struct Icons final {
    /**
     * Optional set of sized icons that the client can display in a user interface.
     *
     * Clients that support rendering icons MUST support at least the following MIME types:
     * - `image/png` - PNG images (safe, universal compatibility)
     * - `image/jpeg` (and `image/jpg`) - JPEG images (safe, universal compatibility)
     *
     * Clients that support rendering icons SHOULD also support:
     * - `image/svg+xml` - SVG images (scalable but requires security precautions)
     * - `image/webp` - WebP images (modern, efficient format)
     */
    std::vector<Icon> icons;
};

/**
 * Base interface for metadata with name (identifier) and title (display name) properties.
 */
struct BaseMetadata final {
    /**
     * Intended for programmatic or logical use, but used as a display name in past specs or
     * fallback (if title isn't present).
     */
    std::string name;
    /**
     * Intended for UI and end-user contexts 
 optimized to be human-readable and easily
     * understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for {@link Tool},
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::string title;
};

enum class ProgressNotification_method : int { notificationsprogress };

/**
 * Parameters for a {@link ProgressNotificationnotifications/progress} notification.
 */
struct ProgressNotificationParams final {
    /**
     * Extends {@link MetaObject} with additional notification-specific fields. All key naming
     * rules from `MetaObject` apply.
     */
    NotificationMetaObject _meta;
    /**
     * An optional message describing the current progress.
     */
    std::string message;
    /**
     * The progress thus far. This should increase every time progress is made, even if the
     * total is unknown.
     */
    double progress;
    /**
     * The progress token which was given in the initial request, used to associate this
     * notification with the request that is proceeding.
     */
    RequestId progressToken;
    /**
     * Total number of items to process (or total progress required), if known.
     */
    double total;
};

/**
 * An out-of-band notification used to inform the receiver of a progress update for a
 * long-running request.
 */
struct ProgressNotification final {
    jsonrpc jsonrpc;
    ProgressNotification_method method;
    ProgressNotificationParams params;
};

/**
 * Common params for paginated requests.
 */
struct PaginatedRequestParams final {
    RequestMetaObject _meta;
    /**
     * An opaque token representing the current pagination position.
     * If provided, the server should return results starting after this cursor.
     */
    std::string cursor;
};

struct PaginatedRequest final {
    RequestId id;
    jsonrpc jsonrpc;
    std::string method;
    PaginatedRequestParams params;
};

struct PaginatedResult final {
    /**
     * Extends {@link MetaObject} with additional result-specific fields. All key naming rules
     * from `MetaObject` apply.
     */
    ResultMetaObject _meta;
    /**
     * An opaque token representing the pagination position after the last returned result.
     * If present, there may be more results available.
     */
    std::string nextCursor;
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    std::string resultType;
};

/**
 * A result that supports a time-to-live (TTL) hint for client-side caching.
 */
struct CacheableResult final {
    /**
     * Extends {@link MetaObject} with additional result-specific fields. All key naming rules
     * from `MetaObject` apply.
     */
    ResultMetaObject _meta;
    /**
     * Indicates the intended scope of the cached response, analogous to HTTP
     * `Cache-Control: public` vs `Cache-Control: private`.
     *
     * - `"public"`: The response does not contain user-specific data. Any
     * client or intermediary (e.g., shared gateway, caching proxy) MAY cache
     * the response and serve it across authorization contexts.
     * - `"private"`: The response MAY be cached and reused only within the
     * same authorization context. Caches MUST NOT be shared across
     * authorization contexts (e.g., a different access token requires a
     * different cache).
     */
    cacheScope cacheScope;
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    std::string resultType;
    /**
     * A hint from the server indicating how long (in milliseconds) the
     * client MAY cache this response before re-fetching. Semantics are
     * analogous to HTTP Cache-Control max-age.
     *
     * - If 0, The response SHOULD be considered immediately stale,
     * The client MAY re-fetch every time the result is needed.
     * - If positive, the client SHOULD consider the result fresh for this many
     * milliseconds after receiving the response.
     */
    double ttlMs;
};

enum class ListResourcesRequest_method : int { resourceslist };

/**
 * Sent from the client to request a list of resources the server has.
 */
struct ListResourcesRequest final {
    RequestId id;
    jsonrpc jsonrpc;
    ListResourcesRequest_method method;
    PaginatedRequestParams params;
};

/**
 * A known resource that the server is capable of reading.
 */
struct Resource final {
    /**
     * Represents the contents of a `_meta` field, which clients and servers use to attach
     * additional metadata to their interactions.
     *
     * Certain key names are reserved by MCP for protocol-level metadata; implementations MUST
     * NOT make assumptions about values at these keys. Additionally, specific schema
     * definitions may reserve particular names for purpose-specific metadata, as declared in
     * those definitions.
     *
     * Valid keys have two segments:
     *
     * **Prefix:**
     * - Optional 
 if specified, MUST be a series of _labels_ separated by dots (`.`), followed
     * by a slash (`/`).
     * - Labels MUST start with a letter and end with a letter or digit. Interior characters may
     * be letters, digits, or hyphens (`-`).
     * - Implementations SHOULD use reverse DNS notation (e.g., `com.example/` rather than
     * `example.com/`).
     * - Any prefix where the second label is `modelcontextprotocol` or `mcp` is **reserved**
     * for MCP use. For example: `io.modelcontextprotocol/`, `dev.mcp/`,
     * `org.modelcontextprotocol.api/`, and `com.mcp.tools/` are all reserved. However,
     * `com.example.mcp/` is NOT reserved, as the second label is `example`.
     *
     * **Name:**
     * - Unless empty, MUST start and end with an alphanumeric character (`[a-z0-9A-Z]`).
     * - Interior characters may be alphanumeric, hyphens (`-`), underscores (`_`), or dots
     * (`.`).
     */
    MetaObject _meta;
    /**
     * Optional annotations for the client.
     */
    Annotations annotations;
    /**
     * A description of what this resource represents.
     *
     * This can be used by clients to improve the LLM's understanding of available resources. It
     * can be thought of like a "hint" to the model.
     */
    std::string description;
    /**
     * Optional set of sized icons that the client can display in a user interface.
     *
     * Clients that support rendering icons MUST support at least the following MIME types:
     * - `image/png` - PNG images (safe, universal compatibility)
     * - `image/jpeg` (and `image/jpg`) - JPEG images (safe, universal compatibility)
     *
     * Clients that support rendering icons SHOULD also support:
     * - `image/svg+xml` - SVG images (scalable but requires security precautions)
     * - `image/webp` - WebP images (modern, efficient format)
     */
    std::vector<Icon> icons;
    /**
     * The MIME type of this resource, if known.
     */
    std::string mimeType;
    /**
     * Intended for programmatic or logical use, but used as a display name in past specs or
     * fallback (if title isn't present).
     */
    std::string name;
    /**
     * The size of the raw resource content, in bytes (i.e., before base64 encoding or any
     * tokenization), if known.
     *
     * This can be used by Hosts to display file sizes and estimate context window usage.
     */
    double size;
    /**
     * Intended for UI and end-user contexts 
 optimized to be human-readable and easily
     * understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for {@link Tool},
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::string title;
    /**
     * The URI of this resource.
     */
    std::string uri;
};

/**
 * The result returned by the server for a {@link ListResourcesRequestresources/list}
 * request.
 */
struct ListResourcesResult final {
    /**
     * Extends {@link MetaObject} with additional result-specific fields. All key naming rules
     * from `MetaObject` apply.
     */
    ResultMetaObject _meta;
    /**
     * Indicates the intended scope of the cached response, analogous to HTTP
     * `Cache-Control: public` vs `Cache-Control: private`.
     *
     * - `"public"`: The response does not contain user-specific data. Any
     * client or intermediary (e.g., shared gateway, caching proxy) MAY cache
     * the response and serve it across authorization contexts.
     * - `"private"`: The response MAY be cached and reused only within the
     * same authorization context. Caches MUST NOT be shared across
     * authorization contexts (e.g., a different access token requires a
     * different cache).
     */
    cacheScope cacheScope;
    /**
     * An opaque token representing the pagination position after the last returned result.
     * If present, there may be more results available.
     */
    std::string nextCursor;
    std::vector<Resource> resources;
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    std::string resultType;
    /**
     * A hint from the server indicating how long (in milliseconds) the
     * client MAY cache this response before re-fetching. Semantics are
     * analogous to HTTP Cache-Control max-age.
     *
     * - If 0, The response SHOULD be considered immediately stale,
     * The client MAY re-fetch every time the result is needed.
     * - If positive, the client SHOULD consider the result fresh for this many
     * milliseconds after receiving the response.
     */
    double ttlMs;
};

/**
 * A successful response from the server for a {@link ListResourcesRequestresources/list}
 * request.
 */
struct ListResourcesResultResponse final {
    RequestId id;
    jsonrpc jsonrpc;
    ListResourcesResult result;
};

enum class ListResourceTemplatesRequest_method : int { resourcestemplateslist };

/**
 * Sent from the client to request a list of resource templates the server has.
 */
struct ListResourceTemplatesRequest final {
    RequestId id;
    jsonrpc jsonrpc;
    ListResourceTemplatesRequest_method method;
    PaginatedRequestParams params;
};

/**
 * A template description for resources available on the server.
 */
struct ResourceTemplate final {
    /**
     * Represents the contents of a `_meta` field, which clients and servers use to attach
     * additional metadata to their interactions.
     *
     * Certain key names are reserved by MCP for protocol-level metadata; implementations MUST
     * NOT make assumptions about values at these keys. Additionally, specific schema
     * definitions may reserve particular names for purpose-specific metadata, as declared in
     * those definitions.
     *
     * Valid keys have two segments:
     *
     * **Prefix:**
     * - Optional 
 if specified, MUST be a series of _labels_ separated by dots (`.`), followed
     * by a slash (`/`).
     * - Labels MUST start with a letter and end with a letter or digit. Interior characters may
     * be letters, digits, or hyphens (`-`).
     * - Implementations SHOULD use reverse DNS notation (e.g., `com.example/` rather than
     * `example.com/`).
     * - Any prefix where the second label is `modelcontextprotocol` or `mcp` is **reserved**
     * for MCP use. For example: `io.modelcontextprotocol/`, `dev.mcp/`,
     * `org.modelcontextprotocol.api/`, and `com.mcp.tools/` are all reserved. However,
     * `com.example.mcp/` is NOT reserved, as the second label is `example`.
     *
     * **Name:**
     * - Unless empty, MUST start and end with an alphanumeric character (`[a-z0-9A-Z]`).
     * - Interior characters may be alphanumeric, hyphens (`-`), underscores (`_`), or dots
     * (`.`).
     */
    MetaObject _meta;
    /**
     * Optional annotations for the client.
     */
    Annotations annotations;
    /**
     * A description of what this template is for.
     *
     * This can be used by clients to improve the LLM's understanding of available resources. It
     * can be thought of like a "hint" to the model.
     */
    std::string description;
    /**
     * Optional set of sized icons that the client can display in a user interface.
     *
     * Clients that support rendering icons MUST support at least the following MIME types:
     * - `image/png` - PNG images (safe, universal compatibility)
     * - `image/jpeg` (and `image/jpg`) - JPEG images (safe, universal compatibility)
     *
     * Clients that support rendering icons SHOULD also support:
     * - `image/svg+xml` - SVG images (scalable but requires security precautions)
     * - `image/webp` - WebP images (modern, efficient format)
     */
    std::vector<Icon> icons;
    /**
     * The MIME type for all resources that match this template. This should only be included if
     * all resources matching this template have the same type.
     */
    std::string mimeType;
    /**
     * Intended for programmatic or logical use, but used as a display name in past specs or
     * fallback (if title isn't present).
     */
    std::string name;
    /**
     * Intended for UI and end-user contexts 
 optimized to be human-readable and easily
     * understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for {@link Tool},
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::string title;
    /**
     * A URI template (according to RFC 6570) that can be used to construct resource URIs.
     */
    std::string uriTemplate;
};

/**
 * The result returned by the server for a {@link
 * ListResourceTemplatesRequestresources/templates/list} request.
 */
struct ListResourceTemplatesResult final {
    /**
     * Extends {@link MetaObject} with additional result-specific fields. All key naming rules
     * from `MetaObject` apply.
     */
    ResultMetaObject _meta;
    /**
     * Indicates the intended scope of the cached response, analogous to HTTP
     * `Cache-Control: public` vs `Cache-Control: private`.
     *
     * - `"public"`: The response does not contain user-specific data. Any
     * client or intermediary (e.g., shared gateway, caching proxy) MAY cache
     * the response and serve it across authorization contexts.
     * - `"private"`: The response MAY be cached and reused only within the
     * same authorization context. Caches MUST NOT be shared across
     * authorization contexts (e.g., a different access token requires a
     * different cache).
     */
    cacheScope cacheScope;
    /**
     * An opaque token representing the pagination position after the last returned result.
     * If present, there may be more results available.
     */
    std::string nextCursor;
    std::vector<ResourceTemplate> resourceTemplates;
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    std::string resultType;
    /**
     * A hint from the server indicating how long (in milliseconds) the
     * client MAY cache this response before re-fetching. Semantics are
     * analogous to HTTP Cache-Control max-age.
     *
     * - If 0, The response SHOULD be considered immediately stale,
     * The client MAY re-fetch every time the result is needed.
     * - If positive, the client SHOULD consider the result fresh for this many
     * milliseconds after receiving the response.
     */
    double ttlMs;
};

/**
 * A successful response from the server for a {@link
 * ListResourceTemplatesRequestresources/templates/list} request.
 */
struct ListResourceTemplatesResultResponse final {
    RequestId id;
    jsonrpc jsonrpc;
    ListResourceTemplatesResult result;
};

/**
 * Common params for resource-related requests.
 */
struct ResourceRequestParams final {
    RequestMetaObject _meta;
    /**
     * The URI of the resource. The URI can use any protocol; it is up to the server how to
     * interpret it.
     */
    std::string uri;
};

enum class ReadResourceRequest_method : int { resourcesread };

/**
 * Parameters for a `resources/read` request.
 */
struct ReadResourceRequestParams final {
    RequestMetaObject _meta;
    /**
     * A map of client responses to server-initiated requests.
     * Keys correspond to the keys in the {@link InputRequests} map;
     * values are the client's result for each request.
     */
    std::map<std::string, InputResponse> inputResponses;
    std::string requestState;
    /**
     * The URI of the resource. The URI can use any protocol; it is up to the server how to
     * interpret it.
     */
    std::string uri;
};

/**
 * Sent from the client to the server, to read a specific resource URI.
 */
struct ReadResourceRequest final {
    RequestId id;
    jsonrpc jsonrpc;
    ReadResourceRequest_method method;
    ReadResourceRequestParams params;
};

/**
 * The result returned by the server for a {@link ReadResourceRequestresources/read} request.
 */
struct ReadResourceResult final {
    /**
     * Extends {@link MetaObject} with additional result-specific fields. All key naming rules
     * from `MetaObject` apply.
     */
    ResultMetaObject _meta;
    /**
     * Indicates the intended scope of the cached response, analogous to HTTP
     * `Cache-Control: public` vs `Cache-Control: private`.
     *
     * - `"public"`: The response does not contain user-specific data. Any
     * client or intermediary (e.g., shared gateway, caching proxy) MAY cache
     * the response and serve it across authorization contexts.
     * - `"private"`: The response MAY be cached and reused only within the
     * same authorization context. Caches MUST NOT be shared across
     * authorization contexts (e.g., a different access token requires a
     * different cache).
     */
    cacheScope cacheScope;
    std::vector<resource> contents;
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    std::string resultType;
    /**
     * A hint from the server indicating how long (in milliseconds) the
     * client MAY cache this response before re-fetching. Semantics are
     * analogous to HTTP Cache-Control max-age.
     *
     * - If 0, The response SHOULD be considered immediately stale,
     * The client MAY re-fetch every time the result is needed.
     * - If positive, the client SHOULD consider the result fresh for this many
     * milliseconds after receiving the response.
     */
    double ttlMs;
};

/**
 * An InputRequiredResult sent by the server to indicate that additional input is needed
 * before the request can be completed.
 *
 * At least one of `inputRequests` or `requestState` MUST be present.
 *
 * The result returned by the server for a {@link ReadResourceRequestresources/read} request.
 */
struct ReadResourceResultResponse_result final {
    /**
     * Extends {@link MetaObject} with additional result-specific fields. All key naming rules
     * from `MetaObject` apply.
     */
    ResultMetaObject _meta;
    /**
     * A map of server-initiated requests that the client must fulfill.
     * Keys are server-assigned identifiers; values are the request objects.
     */
    std::map<std::string, InputRequest> inputRequests;
    std::string requestState;
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    std::string resultType;
    /**
     * Indicates the intended scope of the cached response, analogous to HTTP
     * `Cache-Control: public` vs `Cache-Control: private`.
     *
     * - `"public"`: The response does not contain user-specific data. Any
     * client or intermediary (e.g., shared gateway, caching proxy) MAY cache
     * the response and serve it across authorization contexts.
     * - `"private"`: The response MAY be cached and reused only within the
     * same authorization context. Caches MUST NOT be shared across
     * authorization contexts (e.g., a different access token requires a
     * different cache).
     */
    cacheScope cacheScope;
    std::vector<resource> contents;
    /**
     * A hint from the server indicating how long (in milliseconds) the
     * client MAY cache this response before re-fetching. Semantics are
     * analogous to HTTP Cache-Control max-age.
     *
     * - If 0, The response SHOULD be considered immediately stale,
     * The client MAY re-fetch every time the result is needed.
     * - If positive, the client SHOULD consider the result fresh for this many
     * milliseconds after receiving the response.
     */
    double ttlMs;
};

/**
 * A successful response from the server for a {@link ReadResourceRequestresources/read}
 * request.
 */
struct ReadResourceResultResponse final {
    RequestId id;
    jsonrpc jsonrpc;
    ReadResourceResultResponse_result result;
};

enum class ResourceListChangedNotification_method : int { notificationsresourceslist_changed };

/**
 * Common params for any notification.
 */
struct NotificationParams final {
    /**
     * Extends {@link MetaObject} with additional notification-specific fields. All key naming
     * rules from `MetaObject` apply.
     */
    NotificationMetaObject _meta;
};

/**
 * An optional notification from the server to the client, informing it that the list of
 * resources it can read from has changed. This is only delivered on a {@link
 * SubscriptionsListenRequestsubscriptions/listen} stream when the client requested it via
 * the `resourcesListChanged` filter field.
 */
struct ResourceListChangedNotification final {
    jsonrpc jsonrpc;
    ResourceListChangedNotification_method method;
    /**
     * Common params for any notification.
     */
    NotificationParams params;
};

enum class SubscriptionsListenRequest_method : int { subscriptionslisten };

/**
 * The set of notification types a client may opt in to on a
 * {@link SubscriptionsListenRequestsubscriptions/listen} request.
 *
 * Each notification type is **opt-in**; the server **MUST NOT** send
 * notification types the client has not explicitly requested here.
 *
 * The notifications the client opts in to on this stream. The server
 * **MUST NOT** send notification types the client has not explicitly
 * requested.
 *
 * The subset of requested notification types the server agreed to honor.
 * Only includes notification types the server actually supports; if the
 * client requested an unsupported type (e.g., `promptsListChanged` when
 * the server has no prompts), it is omitted from this set.
 */
struct SubscriptionFilter final {
    /**
     * If true, receive {@link PromptListChangedNotificationnotifications/prompts/list_changed}.
     */
    bool promptsListChanged;
    /**
     * If true, receive {@link
     * ResourceListChangedNotificationnotifications/resources/list_changed}.
     */
    bool resourcesListChanged;
    /**
     * Subscribe to {@link ResourceUpdatedNotificationnotifications/resources/updated} for these
     * resource URIs.
     * Replaces the former `resources/subscribe` RPC.
     */
    std::vector<std::string> resourceSubscriptions;
    /**
     * If true, receive {@link ToolListChangedNotificationnotifications/tools/list_changed}.
     */
    bool toolsListChanged;
};

/**
 * Parameters for a {@link SubscriptionsListenRequestsubscriptions/listen} request.
 */
struct SubscriptionsListenRequestParams final {
    RequestMetaObject _meta;
    /**
     * The notifications the client opts in to on this stream. The server
     * **MUST NOT** send notification types the client has not explicitly
     * requested.
     */
    SubscriptionFilter notifications;
};

/**
 * Sent from the client to open a long-lived channel for receiving notifications
 * outside the context of a specific request. Replaces the previous HTTP GET
 * endpoint and ensures consistent behavior between HTTP and STDIO.
 */
struct SubscriptionsListenRequest final {
    RequestId id;
    jsonrpc jsonrpc;
    SubscriptionsListenRequest_method method;
    SubscriptionsListenRequestParams params;
};

/**
 * Extends {@link ResultMetaObject} with the subscription-stream identifier carried by a
 * {@link SubscriptionsListenResult}. All key naming rules from `MetaObject` apply.
 */
struct SubscriptionsListenResultMetaObject final {
    /**
     * Identifies the server software producing the response. Servers SHOULD
     * include this field on every response unless specifically configured not
     * to do so.
     *
     * The {@link Implementation} schema requires `name` and `version`; other
     * fields are optional.
     *
     * The value is self-reported by the server and is not verified by the
     * protocol. It is intended for display, logging, and debugging. Clients
     * SHOULD NOT use it to change their behavior, and SHOULD NOT rely on it for
     * security decisions.
     */
    Implementation iomodelcontextprotocolserverInfo;
    /**
     * Identifies the subscription stream this response closes, so the client can
     * correlate it with the originating subscription 
 mirroring the same key on
     * the stream's notifications. The value is the JSON-RPC ID of the
     * `subscriptions/listen` request that opened the stream (and equals this
     * response's `id`).
     */
    RequestId iomodelcontextprotocolsubscriptionId;
};

/**
 * The response to a {@link SubscriptionsListenRequestsubscriptions/listen}
 * request, signalling that the subscription has ended gracefully (for example,
 * during server shutdown). Because the listen stream is long-lived, this result
 * is sent only when the server tears the subscription down; an abrupt transport
 * close carries no response. The result body is otherwise empty.
 */
struct SubscriptionsListenResult final {
    SubscriptionsListenResultMetaObject _meta;
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    std::string resultType;
};

/**
 * A successful response from the server for a {@link
 * SubscriptionsListenRequestsubscriptions/listen}
 * request, sent when the server tears the subscription down gracefully.
 */
struct SubscriptionsListenResultResponse final {
    RequestId id;
    jsonrpc jsonrpc;
    SubscriptionsListenResult result;
};

enum class SubscriptionsAcknowledgedNotification_method : int { notificationssubscriptionsacknowledged };

/**
 * Parameters for a {@link
 * SubscriptionsAcknowledgedNotificationnotifications/subscriptions/acknowledged}
 * notification.
 */
struct SubscriptionsAcknowledgedNotificationParams final {
    /**
     * Extends {@link MetaObject} with additional notification-specific fields. All key naming
     * rules from `MetaObject` apply.
     */
    NotificationMetaObject _meta;
    /**
     * The subset of requested notification types the server agreed to honor.
     * Only includes notification types the server actually supports; if the
     * client requested an unsupported type (e.g., `promptsListChanged` when
     * the server has no prompts), it is omitted from this set.
     */
    SubscriptionFilter notifications;
};

/**
 * Sent by the server to acknowledge that a
 * {@link SubscriptionsListenRequestsubscriptions/listen} subscription has been
 * established and to report which notification types it agreed to honor.
 *
 * This notification MUST be the first message the server sends carrying the
 * subscription's ID in `io.modelcontextprotocol/subscriptionId`. The server MUST
 * NOT send any notification on the subscription before acknowledging it. On
 * stdio, where every subscription shares one channel, this ordering is defined
 * per subscription ID and not per channel: messages belonging to other
 * subscriptions MAY be interleaved before it.
 */
struct SubscriptionsAcknowledgedNotification final {
    jsonrpc jsonrpc;
    SubscriptionsAcknowledgedNotification_method method;
    SubscriptionsAcknowledgedNotificationParams params;
};

enum class ResourceUpdatedNotification_method : int { notificationsresourcesupdated };

/**
 * Parameters for a `notifications/resources/updated` notification.
 */
struct ResourceUpdatedNotificationParams final {
    /**
     * Extends {@link MetaObject} with additional notification-specific fields. All key naming
     * rules from `MetaObject` apply.
     */
    NotificationMetaObject _meta;
    /**
     * The URI of the resource that has been updated. This might be a sub-resource of the one
     * that the client actually subscribed to.
     */
    std::string uri;
};

/**
 * A notification from the server to the client, informing it that a resource has changed
 * and may need to be read again. This is only sent for resources the client opted in to via
 * the `resourceSubscriptions` field of a {@link
 * SubscriptionsListenRequestsubscriptions/listen} request.
 */
struct ResourceUpdatedNotification final {
    jsonrpc jsonrpc;
    ResourceUpdatedNotification_method method;
    ResourceUpdatedNotificationParams params;
};

/**
 * The contents of a specific resource or sub-resource.
 */
struct ResourceContents final {
    /**
     * Represents the contents of a `_meta` field, which clients and servers use to attach
     * additional metadata to their interactions.
     *
     * Certain key names are reserved by MCP for protocol-level metadata; implementations MUST
     * NOT make assumptions about values at these keys. Additionally, specific schema
     * definitions may reserve particular names for purpose-specific metadata, as declared in
     * those definitions.
     *
     * Valid keys have two segments:
     *
     * **Prefix:**
     * - Optional 
 if specified, MUST be a series of _labels_ separated by dots (`.`), followed
     * by a slash (`/`).
     * - Labels MUST start with a letter and end with a letter or digit. Interior characters may
     * be letters, digits, or hyphens (`-`).
     * - Implementations SHOULD use reverse DNS notation (e.g., `com.example/` rather than
     * `example.com/`).
     * - Any prefix where the second label is `modelcontextprotocol` or `mcp` is **reserved**
     * for MCP use. For example: `io.modelcontextprotocol/`, `dev.mcp/`,
     * `org.modelcontextprotocol.api/`, and `com.mcp.tools/` are all reserved. However,
     * `com.example.mcp/` is NOT reserved, as the second label is `example`.
     *
     * **Name:**
     * - Unless empty, MUST start and end with an alphanumeric character (`[a-z0-9A-Z]`).
     * - Interior characters may be alphanumeric, hyphens (`-`), underscores (`_`), or dots
     * (`.`).
     */
    MetaObject _meta;
    /**
     * The MIME type of this resource, if known.
     */
    std::string mimeType;
    /**
     * The URI of this resource.
     */
    std::string uri;
};

struct TextResourceContents final {
    /**
     * Represents the contents of a `_meta` field, which clients and servers use to attach
     * additional metadata to their interactions.
     *
     * Certain key names are reserved by MCP for protocol-level metadata; implementations MUST
     * NOT make assumptions about values at these keys. Additionally, specific schema
     * definitions may reserve particular names for purpose-specific metadata, as declared in
     * those definitions.
     *
     * Valid keys have two segments:
     *
     * **Prefix:**
     * - Optional 
 if specified, MUST be a series of _labels_ separated by dots (`.`), followed
     * by a slash (`/`).
     * - Labels MUST start with a letter and end with a letter or digit. Interior characters may
     * be letters, digits, or hyphens (`-`).
     * - Implementations SHOULD use reverse DNS notation (e.g., `com.example/` rather than
     * `example.com/`).
     * - Any prefix where the second label is `modelcontextprotocol` or `mcp` is **reserved**
     * for MCP use. For example: `io.modelcontextprotocol/`, `dev.mcp/`,
     * `org.modelcontextprotocol.api/`, and `com.mcp.tools/` are all reserved. However,
     * `com.example.mcp/` is NOT reserved, as the second label is `example`.
     *
     * **Name:**
     * - Unless empty, MUST start and end with an alphanumeric character (`[a-z0-9A-Z]`).
     * - Interior characters may be alphanumeric, hyphens (`-`), underscores (`_`), or dots
     * (`.`).
     */
    MetaObject _meta;
    /**
     * The MIME type of this resource, if known.
     */
    std::string mimeType;
    /**
     * The text of the item. This must only be set if the item can actually be represented as
     * text (not binary data).
     */
    std::string text;
    /**
     * The URI of this resource.
     */
    std::string uri;
};

struct BlobResourceContents final {
    /**
     * Represents the contents of a `_meta` field, which clients and servers use to attach
     * additional metadata to their interactions.
     *
     * Certain key names are reserved by MCP for protocol-level metadata; implementations MUST
     * NOT make assumptions about values at these keys. Additionally, specific schema
     * definitions may reserve particular names for purpose-specific metadata, as declared in
     * those definitions.
     *
     * Valid keys have two segments:
     *
     * **Prefix:**
     * - Optional 
 if specified, MUST be a series of _labels_ separated by dots (`.`), followed
     * by a slash (`/`).
     * - Labels MUST start with a letter and end with a letter or digit. Interior characters may
     * be letters, digits, or hyphens (`-`).
     * - Implementations SHOULD use reverse DNS notation (e.g., `com.example/` rather than
     * `example.com/`).
     * - Any prefix where the second label is `modelcontextprotocol` or `mcp` is **reserved**
     * for MCP use. For example: `io.modelcontextprotocol/`, `dev.mcp/`,
     * `org.modelcontextprotocol.api/`, and `com.mcp.tools/` are all reserved. However,
     * `com.example.mcp/` is NOT reserved, as the second label is `example`.
     *
     * **Name:**
     * - Unless empty, MUST start and end with an alphanumeric character (`[a-z0-9A-Z]`).
     * - Interior characters may be alphanumeric, hyphens (`-`), underscores (`_`), or dots
     * (`.`).
     */
    MetaObject _meta;
    /**
     * A base64-encoded string representing the binary data of the item.
     */
    std::string blob;
    /**
     * The MIME type of this resource, if known.
     */
    std::string mimeType;
    /**
     * The URI of this resource.
     */
    std::string uri;
};

enum class ListPromptsRequest_method : int { promptslist };

/**
 * Sent from the client to request a list of prompts and prompt templates the server has.
 */
struct ListPromptsRequest final {
    RequestId id;
    jsonrpc jsonrpc;
    ListPromptsRequest_method method;
    PaginatedRequestParams params;
};

/**
 * Describes an argument that a prompt can accept.
 */
struct PromptArgument final {
    /**
     * A human-readable description of the argument.
     */
    std::string description;
    /**
     * Intended for programmatic or logical use, but used as a display name in past specs or
     * fallback (if title isn't present).
     */
    std::string name;
    /**
     * Whether this argument must be provided.
     */
    bool required;
    /**
     * Intended for UI and end-user contexts 
 optimized to be human-readable and easily
     * understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for {@link Tool},
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::string title;
};

/**
 * A prompt or prompt template that the server offers.
 */
struct Prompt final {
    /**
     * Represents the contents of a `_meta` field, which clients and servers use to attach
     * additional metadata to their interactions.
     *
     * Certain key names are reserved by MCP for protocol-level metadata; implementations MUST
     * NOT make assumptions about values at these keys. Additionally, specific schema
     * definitions may reserve particular names for purpose-specific metadata, as declared in
     * those definitions.
     *
     * Valid keys have two segments:
     *
     * **Prefix:**
     * - Optional 
 if specified, MUST be a series of _labels_ separated by dots (`.`), followed
     * by a slash (`/`).
     * - Labels MUST start with a letter and end with a letter or digit. Interior characters may
     * be letters, digits, or hyphens (`-`).
     * - Implementations SHOULD use reverse DNS notation (e.g., `com.example/` rather than
     * `example.com/`).
     * - Any prefix where the second label is `modelcontextprotocol` or `mcp` is **reserved**
     * for MCP use. For example: `io.modelcontextprotocol/`, `dev.mcp/`,
     * `org.modelcontextprotocol.api/`, and `com.mcp.tools/` are all reserved. However,
     * `com.example.mcp/` is NOT reserved, as the second label is `example`.
     *
     * **Name:**
     * - Unless empty, MUST start and end with an alphanumeric character (`[a-z0-9A-Z]`).
     * - Interior characters may be alphanumeric, hyphens (`-`), underscores (`_`), or dots
     * (`.`).
     */
    MetaObject _meta;
    /**
     * A list of arguments to use for templating the prompt.
     */
    std::vector<PromptArgument> arguments;
    /**
     * An optional description of what this prompt provides
     */
    std::string description;
    /**
     * Optional set of sized icons that the client can display in a user interface.
     *
     * Clients that support rendering icons MUST support at least the following MIME types:
     * - `image/png` - PNG images (safe, universal compatibility)
     * - `image/jpeg` (and `image/jpg`) - JPEG images (safe, universal compatibility)
     *
     * Clients that support rendering icons SHOULD also support:
     * - `image/svg+xml` - SVG images (scalable but requires security precautions)
     * - `image/webp` - WebP images (modern, efficient format)
     */
    std::vector<Icon> icons;
    /**
     * Intended for programmatic or logical use, but used as a display name in past specs or
     * fallback (if title isn't present).
     */
    std::string name;
    /**
     * Intended for UI and end-user contexts 
 optimized to be human-readable and easily
     * understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for {@link Tool},
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::string title;
};

/**
 * The result returned by the server for a {@link ListPromptsRequestprompts/list} request.
 */
struct ListPromptsResult final {
    /**
     * Extends {@link MetaObject} with additional result-specific fields. All key naming rules
     * from `MetaObject` apply.
     */
    ResultMetaObject _meta;
    /**
     * Indicates the intended scope of the cached response, analogous to HTTP
     * `Cache-Control: public` vs `Cache-Control: private`.
     *
     * - `"public"`: The response does not contain user-specific data. Any
     * client or intermediary (e.g., shared gateway, caching proxy) MAY cache
     * the response and serve it across authorization contexts.
     * - `"private"`: The response MAY be cached and reused only within the
     * same authorization context. Caches MUST NOT be shared across
     * authorization contexts (e.g., a different access token requires a
     * different cache).
     */
    cacheScope cacheScope;
    /**
     * An opaque token representing the pagination position after the last returned result.
     * If present, there may be more results available.
     */
    std::string nextCursor;
    std::vector<Prompt> prompts;
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    std::string resultType;
    /**
     * A hint from the server indicating how long (in milliseconds) the
     * client MAY cache this response before re-fetching. Semantics are
     * analogous to HTTP Cache-Control max-age.
     *
     * - If 0, The response SHOULD be considered immediately stale,
     * The client MAY re-fetch every time the result is needed.
     * - If positive, the client SHOULD consider the result fresh for this many
     * milliseconds after receiving the response.
     */
    double ttlMs;
};

/**
 * A successful response from the server for a {@link ListPromptsRequestprompts/list}
 * request.
 */
struct ListPromptsResultResponse final {
    RequestId id;
    jsonrpc jsonrpc;
    ListPromptsResult result;
};

enum class GetPromptRequest_method : int { promptsget };

/**
 * Parameters for a `prompts/get` request.
 */
struct GetPromptRequestParams final {
    RequestMetaObject _meta;
    /**
     * Arguments to use for templating the prompt.
     */
    std::map<std::string, std::string> arguments;
    /**
     * A map of client responses to server-initiated requests.
     * Keys correspond to the keys in the {@link InputRequests} map;
     * values are the client's result for each request.
     */
    std::map<std::string, InputResponse> inputResponses;
    /**
     * The name of the prompt or prompt template.
     */
    std::string name;
    std::string requestState;
};

/**
 * Used by the client to get a prompt provided by the server.
 */
struct GetPromptRequest final {
    RequestId id;
    jsonrpc jsonrpc;
    GetPromptRequest_method method;
    GetPromptRequestParams params;
};

/**
 * Describes a message returned as part of a prompt.
 *
 * This is similar to {@link SamplingMessage}, but also supports the embedding of
 * resources from the MCP server.
 */
struct PromptMessage final {
    ContentBlock content;
    Role role;
};

/**
 * The result returned by the server for a {@link GetPromptRequestprompts/get} request.
 */
struct GetPromptResult final {
    /**
     * Extends {@link MetaObject} with additional result-specific fields. All key naming rules
     * from `MetaObject` apply.
     */
    ResultMetaObject _meta;
    /**
     * An optional description for the prompt.
     */
    std::string description;
    std::vector<PromptMessage> messages;
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    std::string resultType;
};

/**
 * An InputRequiredResult sent by the server to indicate that additional input is needed
 * before the request can be completed.
 *
 * At least one of `inputRequests` or `requestState` MUST be present.
 *
 * The result returned by the server for a {@link GetPromptRequestprompts/get} request.
 */
struct GetPromptResultResponse_result final {
    /**
     * Extends {@link MetaObject} with additional result-specific fields. All key naming rules
     * from `MetaObject` apply.
     */
    ResultMetaObject _meta;
    /**
     * A map of server-initiated requests that the client must fulfill.
     * Keys are server-assigned identifiers; values are the request objects.
     */
    std::map<std::string, InputRequest> inputRequests;
    std::string requestState;
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    std::string resultType;
    /**
     * An optional description for the prompt.
     */
    std::string description;
    std::vector<PromptMessage> messages;
};

/**
 * A successful response from the server for a {@link GetPromptRequestprompts/get} request.
 */
struct GetPromptResultResponse final {
    RequestId id;
    jsonrpc jsonrpc;
    GetPromptResultResponse_result result;
};

enum class ResourceLink_type : int { resource_link };

/**
 * A resource that the server is capable of reading, included in a prompt or tool call
 * result.
 *
 * Note: resource links returned by tools are not guaranteed to appear in the results of
 * {@link ListResourcesRequestresources/list} requests.
 */
struct ResourceLink final {
    /**
     * Represents the contents of a `_meta` field, which clients and servers use to attach
     * additional metadata to their interactions.
     *
     * Certain key names are reserved by MCP for protocol-level metadata; implementations MUST
     * NOT make assumptions about values at these keys. Additionally, specific schema
     * definitions may reserve particular names for purpose-specific metadata, as declared in
     * those definitions.
     *
     * Valid keys have two segments:
     *
     * **Prefix:**
     * - Optional 
 if specified, MUST be a series of _labels_ separated by dots (`.`), followed
     * by a slash (`/`).
     * - Labels MUST start with a letter and end with a letter or digit. Interior characters may
     * be letters, digits, or hyphens (`-`).
     * - Implementations SHOULD use reverse DNS notation (e.g., `com.example/` rather than
     * `example.com/`).
     * - Any prefix where the second label is `modelcontextprotocol` or `mcp` is **reserved**
     * for MCP use. For example: `io.modelcontextprotocol/`, `dev.mcp/`,
     * `org.modelcontextprotocol.api/`, and `com.mcp.tools/` are all reserved. However,
     * `com.example.mcp/` is NOT reserved, as the second label is `example`.
     *
     * **Name:**
     * - Unless empty, MUST start and end with an alphanumeric character (`[a-z0-9A-Z]`).
     * - Interior characters may be alphanumeric, hyphens (`-`), underscores (`_`), or dots
     * (`.`).
     */
    MetaObject _meta;
    /**
     * Optional annotations for the client.
     */
    Annotations annotations;
    /**
     * A description of what this resource represents.
     *
     * This can be used by clients to improve the LLM's understanding of available resources. It
     * can be thought of like a "hint" to the model.
     */
    std::string description;
    /**
     * Optional set of sized icons that the client can display in a user interface.
     *
     * Clients that support rendering icons MUST support at least the following MIME types:
     * - `image/png` - PNG images (safe, universal compatibility)
     * - `image/jpeg` (and `image/jpg`) - JPEG images (safe, universal compatibility)
     *
     * Clients that support rendering icons SHOULD also support:
     * - `image/svg+xml` - SVG images (scalable but requires security precautions)
     * - `image/webp` - WebP images (modern, efficient format)
     */
    std::vector<Icon> icons;
    /**
     * The MIME type of this resource, if known.
     */
    std::string mimeType;
    /**
     * Intended for programmatic or logical use, but used as a display name in past specs or
     * fallback (if title isn't present).
     */
    std::string name;
    /**
     * The size of the raw resource content, in bytes (i.e., before base64 encoding or any
     * tokenization), if known.
     *
     * This can be used by Hosts to display file sizes and estimate context window usage.
     */
    double size;
    /**
     * Intended for UI and end-user contexts 
 optimized to be human-readable and easily
     * understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for {@link Tool},
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::string title;
    ResourceLink_type type;
    /**
     * The URI of this resource.
     */
    std::string uri;
};

enum class EmbeddedResource_type : int { resource };

/**
 * The contents of a resource, embedded into a prompt or tool call result.
 *
 * It is up to the client how best to render embedded resources for the benefit
 * of the LLM and/or the user.
 */
struct EmbeddedResource final {
    /**
     * Represents the contents of a `_meta` field, which clients and servers use to attach
     * additional metadata to their interactions.
     *
     * Certain key names are reserved by MCP for protocol-level metadata; implementations MUST
     * NOT make assumptions about values at these keys. Additionally, specific schema
     * definitions may reserve particular names for purpose-specific metadata, as declared in
     * those definitions.
     *
     * Valid keys have two segments:
     *
     * **Prefix:**
     * - Optional 
 if specified, MUST be a series of _labels_ separated by dots (`.`), followed
     * by a slash (`/`).
     * - Labels MUST start with a letter and end with a letter or digit. Interior characters may
     * be letters, digits, or hyphens (`-`).
     * - Implementations SHOULD use reverse DNS notation (e.g., `com.example/` rather than
     * `example.com/`).
     * - Any prefix where the second label is `modelcontextprotocol` or `mcp` is **reserved**
     * for MCP use. For example: `io.modelcontextprotocol/`, `dev.mcp/`,
     * `org.modelcontextprotocol.api/`, and `com.mcp.tools/` are all reserved. However,
     * `com.example.mcp/` is NOT reserved, as the second label is `example`.
     *
     * **Name:**
     * - Unless empty, MUST start and end with an alphanumeric character (`[a-z0-9A-Z]`).
     * - Interior characters may be alphanumeric, hyphens (`-`), underscores (`_`), or dots
     * (`.`).
     */
    MetaObject _meta;
    /**
     * Optional annotations for the client.
     */
    Annotations annotations;
    resource resource;
    EmbeddedResource_type type;
};

enum class PromptListChangedNotification_method : int { notificationspromptslist_changed };

/**
 * An optional notification from the server to the client, informing it that the list of
 * prompts it offers has changed. This is only delivered on a {@link
 * SubscriptionsListenRequestsubscriptions/listen} stream when the client requested it via
 * the `promptsListChanged` filter field.
 */
struct PromptListChangedNotification final {
    jsonrpc jsonrpc;
    PromptListChangedNotification_method method;
    /**
     * Common params for any notification.
     */
    NotificationParams params;
};

enum class ListToolsRequest_method : int { toolslist };

/**
 * Sent from the client to request a list of tools the server has.
 */
struct ListToolsRequest final {
    RequestId id;
    jsonrpc jsonrpc;
    ListToolsRequest_method method;
    PaginatedRequestParams params;
};

/**
 * The result returned by the server for a {@link ListToolsRequesttools/list} request.
 */
struct ListToolsResult final {
    /**
     * Extends {@link MetaObject} with additional result-specific fields. All key naming rules
     * from `MetaObject` apply.
     */
    ResultMetaObject _meta;
    /**
     * Indicates the intended scope of the cached response, analogous to HTTP
     * `Cache-Control: public` vs `Cache-Control: private`.
     *
     * - `"public"`: The response does not contain user-specific data. Any
     * client or intermediary (e.g., shared gateway, caching proxy) MAY cache
     * the response and serve it across authorization contexts.
     * - `"private"`: The response MAY be cached and reused only within the
     * same authorization context. Caches MUST NOT be shared across
     * authorization contexts (e.g., a different access token requires a
     * different cache).
     */
    cacheScope cacheScope;
    /**
     * An opaque token representing the pagination position after the last returned result.
     * If present, there may be more results available.
     */
    std::string nextCursor;
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    std::string resultType;
    std::vector<Tool> tools;
    /**
     * A hint from the server indicating how long (in milliseconds) the
     * client MAY cache this response before re-fetching. Semantics are
     * analogous to HTTP Cache-Control max-age.
     *
     * - If 0, The response SHOULD be considered immediately stale,
     * The client MAY re-fetch every time the result is needed.
     * - If positive, the client SHOULD consider the result fresh for this many
     * milliseconds after receiving the response.
     */
    double ttlMs;
};

/**
 * A successful response from the server for a {@link ListToolsRequesttools/list} request.
 */
struct ListToolsResultResponse final {
    RequestId id;
    jsonrpc jsonrpc;
    ListToolsResult result;
};

/**
 * The result returned by the server for a {@link CallToolRequesttools/call} request.
 */
struct CallToolResult final {
    /**
     * Extends {@link MetaObject} with additional result-specific fields. All key naming rules
     * from `MetaObject` apply.
     */
    ResultMetaObject _meta;
    /**
     * A list of content objects that represent the unstructured result of the tool call.
     */
    std::vector<ContentBlock> content;
    /**
     * Whether the tool call ended in an error.
     *
     * If not set, this is assumed to be false (the call was successful).
     *
     * Any errors that originate from the tool SHOULD be reported inside the result
     * object, with `isError` set to true, _not_ as an MCP protocol-level error
     * response. Otherwise, the LLM would not be able to see that an error occurred
     * and self-correct.
     *
     * However, any errors in _finding_ the tool, an error indicating that the
     * server does not support tool calls, or any other exceptional conditions,
     * should be reported as an MCP error response.
     */
    bool isError;
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    std::string resultType;
    /**
     * An optional JSON value that represents the structured result of the tool call.
     *
     * This can be any JSON value (object, array, string, number, boolean, or null)
     * that conforms to the tool's outputSchema if one is defined.
     */
    JsonValue structuredContent;
};

/**
 * An InputRequiredResult sent by the server to indicate that additional input is needed
 * before the request can be completed.
 *
 * At least one of `inputRequests` or `requestState` MUST be present.
 *
 * The result returned by the server for a {@link CallToolRequesttools/call} request.
 */
struct CallToolResultResponse_result final {
    /**
     * Extends {@link MetaObject} with additional result-specific fields. All key naming rules
     * from `MetaObject` apply.
     */
    ResultMetaObject _meta;
    /**
     * A map of server-initiated requests that the client must fulfill.
     * Keys are server-assigned identifiers; values are the request objects.
     */
    std::map<std::string, InputRequest> inputRequests;
    std::string requestState;
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    std::string resultType;
    /**
     * A list of content objects that represent the unstructured result of the tool call.
     */
    std::vector<ContentBlock> content;
    /**
     * Whether the tool call ended in an error.
     *
     * If not set, this is assumed to be false (the call was successful).
     *
     * Any errors that originate from the tool SHOULD be reported inside the result
     * object, with `isError` set to true, _not_ as an MCP protocol-level error
     * response. Otherwise, the LLM would not be able to see that an error occurred
     * and self-correct.
     *
     * However, any errors in _finding_ the tool, an error indicating that the
     * server does not support tool calls, or any other exceptional conditions,
     * should be reported as an MCP error response.
     */
    bool isError;
    /**
     * An optional JSON value that represents the structured result of the tool call.
     *
     * This can be any JSON value (object, array, string, number, boolean, or null)
     * that conforms to the tool's outputSchema if one is defined.
     */
    JsonValue structuredContent;
};

/**
 * A successful response from the server for a {@link CallToolRequesttools/call} request.
 */
struct CallToolResultResponse final {
    RequestId id;
    jsonrpc jsonrpc;
    CallToolResultResponse_result result;
};

enum class CallToolRequest_method : int { toolscall };

/**
 * Parameters for a `tools/call` request.
 */
struct CallToolRequestParams final {
    RequestMetaObject _meta;
    /**
     * Arguments to use for the tool call.
     */
    std::map<std::string, JsonValue> arguments;
    /**
     * A map of client responses to server-initiated requests.
     * Keys correspond to the keys in the {@link InputRequests} map;
     * values are the client's result for each request.
     */
    std::map<std::string, InputResponse> inputResponses;
    /**
     * The name of the tool.
     */
    std::string name;
    std::string requestState;
};

/**
 * Used by the client to invoke a tool provided by the server.
 */
struct CallToolRequest final {
    RequestId id;
    jsonrpc jsonrpc;
    CallToolRequest_method method;
    CallToolRequestParams params;
};

enum class ToolListChangedNotification_method : int { notificationstoolslist_changed };

/**
 * An optional notification from the server to the client, informing it that the list of
 * tools it offers has changed. This is only delivered on a {@link
 * SubscriptionsListenRequestsubscriptions/listen} stream when the client requested it via
 * the `toolsListChanged` filter field.
 */
struct ToolListChangedNotification final {
    jsonrpc jsonrpc;
    ToolListChangedNotification_method method;
    /**
     * Common params for any notification.
     */
    NotificationParams params;
};

enum class LoggingMessageNotification_method : int { notificationsmessage };

/**
 * Parameters for a `notifications/message` notification.
 */
struct LoggingMessageNotificationParams final {
    /**
     * Extends {@link MetaObject} with additional notification-specific fields. All key naming
     * rules from `MetaObject` apply.
     */
    NotificationMetaObject _meta;
    /**
     * The data to be logged, such as a string message or an object. Any JSON serializable type
     * is allowed here.
     */
    JsonValue data;
    /**
     * The severity of this log message.
     */
    LoggingLevel level;
    /**
     * An optional name of the logger issuing this message.
     */
    std::string logger;
};

/**
 * JSONRPCNotification of a log message passed from server to client. The client opts in by
 * setting `"io.modelcontextprotocol/logLevel"` in a request's `_meta`.
 */
struct LoggingMessageNotification final {
    jsonrpc jsonrpc;
    LoggingMessageNotification_method method;
    LoggingMessageNotificationParams params;
};

enum class CreateMessageRequest_method : int { samplingcreateMessage };

/**
 * Parameters for a `sampling/createMessage` request.
 */
struct CreateMessageRequestParams final {
    /**
     * A request to include context from one or more MCP servers (including the caller), to be
     * attached to the prompt.
     * The client MAY ignore this request.
     *
     * Default is `"none"`. The values `"thisServer"` and `"allServers"` are deprecated
     * (SEP-2596): servers SHOULD
     * omit this field or use `"none"`, and SHOULD only use the deprecated values if the client
     * declares
     * {@link ClientCapabilities.sampling.context}.
     */
    includeContext includeContext;
    /**
     * The requested maximum number of tokens to sample (to prevent runaway completions).
     *
     * The client MAY choose to sample fewer tokens than the requested maximum.
     */
    double maxTokens;
    std::vector<SamplingMessage> messages;
    /**
     * Optional metadata to pass through to the LLM provider. The format of this metadata is
     * provider-specific.
     */
    std::map<std::string, JsonValue> metadata;
    /**
     * The server's preferences for which model to select. The client MAY ignore these
     * preferences.
     */
    ModelPreferences modelPreferences;
    std::vector<std::string> stopSequences;
    /**
     * An optional system prompt the server wants to use for sampling. The client MAY modify or
     * omit this prompt.
     */
    std::string systemPrompt;
    double temperature;
    /**
     * Controls how the model uses tools.
     * The client MUST return an error if this field is provided but {@link
     * ClientCapabilities.sampling.tools} is not declared.
     * Default is `{ mode: "auto" }`.
     */
    ToolChoice toolChoice;
    /**
     * Tools that the model may use during generation.
     * The client MUST return an error if this field is provided but {@link
     * ClientCapabilities.sampling.tools} is not declared.
     */
    std::vector<Tool> tools;
};

/**
 * A request from the server to sample an LLM via the client. The client has full discretion
 * over which model to select. The client should also inform the user before beginning
 * sampling, to allow them to inspect the request (human in the loop) and decide whether to
 * approve it.
 */
struct CreateMessageRequest final {
    CreateMessageRequest_method method;
    CreateMessageRequestParams params;
};

/**
 * The result returned by the client for a {@link
 * CreateMessageRequestsampling/createMessage} request.
 * The client should inform the user before returning the sampled message, to allow them
 * to inspect the response (human in the loop) and decide whether to allow the server to see
 * it.
 */
struct CreateMessageResult final {
    /**
     * Represents the contents of a `_meta` field, which clients and servers use to attach
     * additional metadata to their interactions.
     *
     * Certain key names are reserved by MCP for protocol-level metadata; implementations MUST
     * NOT make assumptions about values at these keys. Additionally, specific schema
     * definitions may reserve particular names for purpose-specific metadata, as declared in
     * those definitions.
     *
     * Valid keys have two segments:
     *
     * **Prefix:**
     * - Optional 
 if specified, MUST be a series of _labels_ separated by dots (`.`), followed
     * by a slash (`/`).
     * - Labels MUST start with a letter and end with a letter or digit. Interior characters may
     * be letters, digits, or hyphens (`-`).
     * - Implementations SHOULD use reverse DNS notation (e.g., `com.example/` rather than
     * `example.com/`).
     * - Any prefix where the second label is `modelcontextprotocol` or `mcp` is **reserved**
     * for MCP use. For example: `io.modelcontextprotocol/`, `dev.mcp/`,
     * `org.modelcontextprotocol.api/`, and `com.mcp.tools/` are all reserved. However,
     * `com.example.mcp/` is NOT reserved, as the second label is `example`.
     *
     * **Name:**
     * - Unless empty, MUST start and end with an alphanumeric character (`[a-z0-9A-Z]`).
     * - Interior characters may be alphanumeric, hyphens (`-`), underscores (`_`), or dots
     * (`.`).
     */
    MetaObject _meta;
    SamplingMessage_content content;
    /**
     * The name of the model that generated the message.
     */
    std::string model;
    Role role;
    /**
     * The reason why sampling stopped, if known.
     *
     * Standard values:
     * - `"endTurn"`: Natural end of the assistant's turn
     * - `"stopSequence"`: A stop sequence was encountered
     * - `"maxTokens"`: Maximum token limit was reached
     * - `"toolUse"`: The model wants to use one or more tools
     *
     * This field is an open string to allow for provider-specific stop reasons.
     */
    std::string stopReason;
};

enum class Purple_type : int { text };

/**
 * Text provided to or from an LLM.
 */
struct TextContent final {
    /**
     * Represents the contents of a `_meta` field, which clients and servers use to attach
     * additional metadata to their interactions.
     *
     * Certain key names are reserved by MCP for protocol-level metadata; implementations MUST
     * NOT make assumptions about values at these keys. Additionally, specific schema
     * definitions may reserve particular names for purpose-specific metadata, as declared in
     * those definitions.
     *
     * Valid keys have two segments:
     *
     * **Prefix:**
     * - Optional 
 if specified, MUST be a series of _labels_ separated by dots (`.`), followed
     * by a slash (`/`).
     * - Labels MUST start with a letter and end with a letter or digit. Interior characters may
     * be letters, digits, or hyphens (`-`).
     * - Implementations SHOULD use reverse DNS notation (e.g., `com.example/` rather than
     * `example.com/`).
     * - Any prefix where the second label is `modelcontextprotocol` or `mcp` is **reserved**
     * for MCP use. For example: `io.modelcontextprotocol/`, `dev.mcp/`,
     * `org.modelcontextprotocol.api/`, and `com.mcp.tools/` are all reserved. However,
     * `com.example.mcp/` is NOT reserved, as the second label is `example`.
     *
     * **Name:**
     * - Unless empty, MUST start and end with an alphanumeric character (`[a-z0-9A-Z]`).
     * - Interior characters may be alphanumeric, hyphens (`-`), underscores (`_`), or dots
     * (`.`).
     */
    MetaObject _meta;
    /**
     * Optional annotations for the client.
     */
    Annotations annotations;
    /**
     * The text content of the message.
     */
    std::string text;
    Purple_type type;
};

enum class Fluffy_type : int { image };

/**
 * An image provided to or from an LLM.
 */
struct ImageContent final {
    /**
     * Represents the contents of a `_meta` field, which clients and servers use to attach
     * additional metadata to their interactions.
     *
     * Certain key names are reserved by MCP for protocol-level metadata; implementations MUST
     * NOT make assumptions about values at these keys. Additionally, specific schema
     * definitions may reserve particular names for purpose-specific metadata, as declared in
     * those definitions.
     *
     * Valid keys have two segments:
     *
     * **Prefix:**
     * - Optional 
 if specified, MUST be a series of _labels_ separated by dots (`.`), followed
     * by a slash (`/`).
     * - Labels MUST start with a letter and end with a letter or digit. Interior characters may
     * be letters, digits, or hyphens (`-`).
     * - Implementations SHOULD use reverse DNS notation (e.g., `com.example/` rather than
     * `example.com/`).
     * - Any prefix where the second label is `modelcontextprotocol` or `mcp` is **reserved**
     * for MCP use. For example: `io.modelcontextprotocol/`, `dev.mcp/`,
     * `org.modelcontextprotocol.api/`, and `com.mcp.tools/` are all reserved. However,
     * `com.example.mcp/` is NOT reserved, as the second label is `example`.
     *
     * **Name:**
     * - Unless empty, MUST start and end with an alphanumeric character (`[a-z0-9A-Z]`).
     * - Interior characters may be alphanumeric, hyphens (`-`), underscores (`_`), or dots
     * (`.`).
     */
    MetaObject _meta;
    /**
     * Optional annotations for the client.
     */
    Annotations annotations;
    /**
     * The base64-encoded image data.
     */
    std::string data;
    /**
     * The MIME type of the image. Different providers may support different image types.
     */
    std::string mimeType;
    Fluffy_type type;
};

enum class Tentacled_type : int { audio };

/**
 * Audio provided to or from an LLM.
 */
struct AudioContent final {
    /**
     * Represents the contents of a `_meta` field, which clients and servers use to attach
     * additional metadata to their interactions.
     *
     * Certain key names are reserved by MCP for protocol-level metadata; implementations MUST
     * NOT make assumptions about values at these keys. Additionally, specific schema
     * definitions may reserve particular names for purpose-specific metadata, as declared in
     * those definitions.
     *
     * Valid keys have two segments:
     *
     * **Prefix:**
     * - Optional 
 if specified, MUST be a series of _labels_ separated by dots (`.`), followed
     * by a slash (`/`).
     * - Labels MUST start with a letter and end with a letter or digit. Interior characters may
     * be letters, digits, or hyphens (`-`).
     * - Implementations SHOULD use reverse DNS notation (e.g., `com.example/` rather than
     * `example.com/`).
     * - Any prefix where the second label is `modelcontextprotocol` or `mcp` is **reserved**
     * for MCP use. For example: `io.modelcontextprotocol/`, `dev.mcp/`,
     * `org.modelcontextprotocol.api/`, and `com.mcp.tools/` are all reserved. However,
     * `com.example.mcp/` is NOT reserved, as the second label is `example`.
     *
     * **Name:**
     * - Unless empty, MUST start and end with an alphanumeric character (`[a-z0-9A-Z]`).
     * - Interior characters may be alphanumeric, hyphens (`-`), underscores (`_`), or dots
     * (`.`).
     */
    MetaObject _meta;
    /**
     * Optional annotations for the client.
     */
    Annotations annotations;
    /**
     * The base64-encoded audio data.
     */
    std::string data;
    /**
     * The MIME type of the audio. Different providers may support different audio types.
     */
    std::string mimeType;
    Tentacled_type type;
};

enum class Sticky_type : int { tool_use };

/**
 * A request from the assistant to call a tool.
 */
struct ToolUseContent final {
    /**
     * Optional metadata about the tool use. Clients SHOULD preserve this field when
     * including tool uses in subsequent sampling requests to enable caching optimizations.
     */
    MetaObject _meta;
    /**
     * A unique identifier for this tool use.
     *
     * This ID is used to match tool results to their corresponding tool uses.
     */
    std::string id;
    /**
     * The arguments to pass to the tool, conforming to the tool's input schema.
     */
    std::map<std::string, JsonValue> input;
    /**
     * The name of the tool to call.
     */
    std::string name;
    Sticky_type type;
};

enum class Indigo_type : int { tool_result };

/**
 * The result of a tool use, provided by the user back to the assistant.
 */
struct ToolResultContent final {
    /**
     * Optional metadata about the tool result. Clients SHOULD preserve this field when
     * including tool results in subsequent sampling requests to enable caching optimizations.
     */
    MetaObject _meta;
    /**
     * The unstructured result content of the tool use.
     *
     * This has the same format as {@link CallToolResult.content} and can include text, images,
     * audio, resource links, and embedded resources.
     */
    std::vector<ContentBlock> content;
    /**
     * Whether the tool use resulted in an error.
     *
     * If true, the content typically describes the error that occurred.
     * Default: false
     */
    bool isError;
    /**
     * An optional structured result value.
     *
     * This can be any JSON value (object, array, string, number, boolean, or null).
     * If the tool defined an {@link Tool.outputSchema}, this SHOULD conform to that schema.
     */
    JsonValue structuredContent;
    /**
     * The ID of the tool use this result corresponds to.
     *
     * This MUST match the ID from a previous {@link ToolUseContent}.
     */
    std::string toolUseId;
    Indigo_type type;
};

enum class CompleteRequest_method : int { completioncomplete };

/**
 * The argument's information
 */
struct argument final {
    /**
     * The name of the argument
     */
    std::string name;
    /**
     * The value of the argument to use for completion matching.
     */
    std::string value;
};

/**
 * Additional, optional context for completions
 */
struct context final {
    /**
     * Previously-resolved variables in a URI template or prompt.
     */
    std::map<std::string, std::string> arguments;
};

enum class ref_type : int { refprompt, refresource };

/**
 * Identifies a prompt.
 *
 * A reference to a resource or resource template definition.
 */
struct ref final {
    /**
     * Intended for programmatic or logical use, but used as a display name in past specs or
     * fallback (if title isn't present).
     */
    std::string name;
    /**
     * Intended for UI and end-user contexts 
 optimized to be human-readable and easily
     * understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for {@link Tool},
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::string title;
    ref_type type;
    /**
     * The URI or URI template of the resource.
     */
    std::string uri;
};

/**
 * Parameters for a `completion/complete` request.
 */
struct CompleteRequestParams final {
    RequestMetaObject _meta;
    /**
     * The argument's information
     */
    argument argument;
    /**
     * Additional, optional context for completions
     */
    context context;
    ref ref;
};

/**
 * A request from the client to the server, to ask for completion options.
 */
struct CompleteRequest final {
    RequestId id;
    jsonrpc jsonrpc;
    CompleteRequest_method method;
    CompleteRequestParams params;
};

struct completion final {
    /**
     * Indicates whether there are additional completion options beyond those provided in the
     * current response, even if the exact total is unknown.
     */
    bool hasMore;
    /**
     * The total number of completion options available. This can exceed the number of values
     * actually sent in the response.
     */
    double total;
    /**
     * An array of completion values. Must not exceed 100 items.
     */
    std::vector<std::string> values;
};

/**
 * The result returned by the server for a {@link CompleteRequestcompletion/complete}
 * request.
 */
struct CompleteResult final {
    /**
     * Extends {@link MetaObject} with additional result-specific fields. All key naming rules
     * from `MetaObject` apply.
     */
    ResultMetaObject _meta;
    completion completion;
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    std::string resultType;
};

/**
 * A successful response from the server for a {@link CompleteRequestcompletion/complete}
 * request.
 */
struct CompleteResultResponse final {
    RequestId id;
    jsonrpc jsonrpc;
    CompleteResult result;
};

enum class ResourceTemplateReference_type : int { refresource };

/**
 * A reference to a resource or resource template definition.
 */
struct ResourceTemplateReference final {
    ResourceTemplateReference_type type;
    /**
     * The URI or URI template of the resource.
     */
    std::string uri;
};

enum class PromptReference_type : int { refprompt };

/**
 * Identifies a prompt.
 */
struct PromptReference final {
    /**
     * Intended for programmatic or logical use, but used as a display name in past specs or
     * fallback (if title isn't present).
     */
    std::string name;
    /**
     * Intended for UI and end-user contexts 
 optimized to be human-readable and easily
     * understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for {@link Tool},
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::string title;
    PromptReference_type type;
};

enum class ListRootsRequest_method : int { rootslist };

struct ListRootsRequest_params final {
    /**
     * Represents the contents of a `_meta` field, which clients and servers use to attach
     * additional metadata to their interactions.
     *
     * Certain key names are reserved by MCP for protocol-level metadata; implementations MUST
     * NOT make assumptions about values at these keys. Additionally, specific schema
     * definitions may reserve particular names for purpose-specific metadata, as declared in
     * those definitions.
     *
     * Valid keys have two segments:
     *
     * **Prefix:**
     * - Optional 
 if specified, MUST be a series of _labels_ separated by dots (`.`), followed
     * by a slash (`/`).
     * - Labels MUST start with a letter and end with a letter or digit. Interior characters may
     * be letters, digits, or hyphens (`-`).
     * - Implementations SHOULD use reverse DNS notation (e.g., `com.example/` rather than
     * `example.com/`).
     * - Any prefix where the second label is `modelcontextprotocol` or `mcp` is **reserved**
     * for MCP use. For example: `io.modelcontextprotocol/`, `dev.mcp/`,
     * `org.modelcontextprotocol.api/`, and `com.mcp.tools/` are all reserved. However,
     * `com.example.mcp/` is NOT reserved, as the second label is `example`.
     *
     * **Name:**
     * - Unless empty, MUST start and end with an alphanumeric character (`[a-z0-9A-Z]`).
     * - Interior characters may be alphanumeric, hyphens (`-`), underscores (`_`), or dots
     * (`.`).
     */
    MetaObject _meta;
};

/**
 * Sent from the server to request a list of root URIs from the client. Roots allow
 * servers to ask for specific directories or files to operate on. A common example
 * for roots is providing a set of repositories or directories a server should operate
 * on.
 *
 * This request is typically used when the server needs to understand the file system
 * structure or access specific locations that the client has permission to read from.
 */
struct ListRootsRequest final {
    ListRootsRequest_method method;
    ListRootsRequest_params params;
};

/**
 * The result returned by the client for a {@link ListRootsRequestroots/list} request.
 * This result contains an array of {@link Root} objects, each representing a root directory
 * or file that the server can operate on.
 */
struct ListRootsResult final {
    std::vector<Root> roots;
};

enum class ElicitRequestFormParams_mode : int { form };

/**
 * The parameters for a request to elicit non-sensitive information from the user via a form
 * in the client.
 */
struct ElicitRequestFormParams final {
    /**
     * The message to present to the user describing what information is being requested.
     */
    std::string message;
    /**
     * The elicitation mode.
     */
    ElicitRequestFormParams_mode mode;
    /**
     * A restricted subset of JSON Schema.
     * Only top-level properties are allowed, without nesting.
     */
    requestedSchema requestedSchema;
};

enum class ElicitRequestURLParams_mode : int { url };

/**
 * The parameters for a request to elicit information from the user via a URL in the client.
 */
struct ElicitRequestURLParams final {
    /**
     * The message to present to the user explaining why the interaction is needed.
     */
    std::string message;
    /**
     * The elicitation mode.
     */
    ElicitRequestURLParams_mode mode;
    /**
     * The URL that the user should navigate to.
     */
    std::string url;
};

enum class ElicitRequest_method : int { elicitationcreate };

/**
 * The parameters for a request to elicit additional information from the user via the
 * client.
 *
 * The parameters for a request to elicit non-sensitive information from the user via a form
 * in the client.
 *
 * The parameters for a request to elicit information from the user via a URL in the client.
 */
struct ElicitRequestParams final {
    /**
     * The message to present to the user describing what information is being requested.
     *
     * The message to present to the user explaining why the interaction is needed.
     */
    std::string message;
    /**
     * The elicitation mode.
     */
    params_mode mode;
    /**
     * A restricted subset of JSON Schema.
     * Only top-level properties are allowed, without nesting.
     */
    requestedSchema requestedSchema;
    /**
     * The URL that the user should navigate to.
     */
    std::string url;
};

/**
 * A request from the server to elicit additional information from the user via the client.
 */
struct ElicitRequest final {
    ElicitRequest_method method;
    ElicitRequestParams params;
};

struct StringSchema final {
    std::string StringSchema_default;
    std::string description;
    format format;
    double maxLength;
    double minLength;
    std::string title;
    items_type type;
};

enum class NumberSchema_type : int { integer, number };

struct NumberSchema final {
    double NumberSchema_default;
    std::string description;
    double maximum;
    double minimum;
    std::string title;
    NumberSchema_type type;
};

enum class BooleanSchema_type : int { boolean };

struct BooleanSchema final {
    bool BooleanSchema_default;
    std::string description;
    std::string title;
    BooleanSchema_type type;
};

/**
 * Schema for single-selection enumeration without display titles for options.
 */
struct UntitledSingleSelectEnumSchema final {
    /**
     * Optional default value.
     */
    std::string UntitledSingleSelectEnumSchema_default;
    /**
     * Optional description for the enum field.
     */
    std::string description;
    /**
     * Array of enum values to choose from.
     */
    std::vector<std::string> UntitledSingleSelectEnumSchema_enum;
    /**
     * Optional title for the enum field.
     */
    std::string title;
    items_type type;
};

/**
 * Schema for single-selection enumeration with display titles for each option.
 */
struct TitledSingleSelectEnumSchema final {
    /**
     * Optional default value.
     */
    std::string TitledSingleSelectEnumSchema_default;
    /**
     * Optional description for the enum field.
     */
    std::string description;
    /**
     * Array of enum options with values and display labels.
     */
    std::vector<oneOf> oneOf;
    /**
     * Optional title for the enum field.
     */
    std::string title;
    items_type type;
};

/**
 * Schema for single-selection enumeration without display titles for options.
 *
 * Schema for single-selection enumeration with display titles for each option.
 */
struct SingleSelectEnumSchema final {
    /**
     * Optional default value.
     */
    std::string SingleSelectEnumSchema_default;
    /**
     * Optional description for the enum field.
     */
    std::string description;
    /**
     * Array of enum values to choose from.
     */
    std::vector<std::string> SingleSelectEnumSchema_enum;
    /**
     * Optional title for the enum field.
     */
    std::string title;
    items_type type;
    /**
     * Array of enum options with values and display labels.
     */
    std::vector<oneOf> oneOf;
};

/**
 * Schema for the array items.
 */
struct UntitledMultiSelectEnumSchema_items final {
    /**
     * Array of enum values to choose from.
     */
    std::vector<std::string> items_enum;
    items_type type;
};

enum class UntitledMultiSelectEnumSchema_type : int { array };

/**
 * Schema for multiple-selection enumeration without display titles for options.
 */
struct UntitledMultiSelectEnumSchema final {
    /**
     * Optional default value.
     */
    std::vector<std::string> UntitledMultiSelectEnumSchema_default;
    /**
     * Optional description for the enum field.
     */
    std::string description;
    /**
     * Schema for the array items.
     */
    UntitledMultiSelectEnumSchema_items items;
    /**
     * Maximum number of items to select.
     */
    double maxItems;
    /**
     * Minimum number of items to select.
     */
    double minItems;
    /**
     * Optional title for the enum field.
     */
    std::string title;
    UntitledMultiSelectEnumSchema_type type;
};

/**
 * Schema for array items with enum options and display labels.
 */
struct TitledMultiSelectEnumSchema_items final {
    /**
     * Array of enum options with values and display labels.
     */
    std::vector<anyOf> anyOf;
};

/**
 * Schema for multiple-selection enumeration with display titles for each option.
 */
struct TitledMultiSelectEnumSchema final {
    /**
     * Optional default value.
     */
    std::vector<std::string> TitledMultiSelectEnumSchema_default;
    /**
     * Optional description for the enum field.
     */
    std::string description;
    /**
     * Schema for array items with enum options and display labels.
     */
    TitledMultiSelectEnumSchema_items items;
    /**
     * Maximum number of items to select.
     */
    double maxItems;
    /**
     * Minimum number of items to select.
     */
    double minItems;
    /**
     * Optional title for the enum field.
     */
    std::string title;
    UntitledMultiSelectEnumSchema_type type;
};

/**
 * Schema for multiple-selection enumeration without display titles for options.
 *
 * Schema for multiple-selection enumeration with display titles for each option.
 */
struct MultiSelectEnumSchema final {
    /**
     * Optional default value.
     */
    std::vector<std::string> MultiSelectEnumSchema_default;
    /**
     * Optional description for the enum field.
     */
    std::string description;
    /**
     * Schema for the array items.
     *
     * Schema for array items with enum options and display labels.
     */
    PrimitiveSchemaDefinition_items items;
    /**
     * Maximum number of items to select.
     */
    double maxItems;
    /**
     * Minimum number of items to select.
     */
    double minItems;
    /**
     * Optional title for the enum field.
     */
    std::string title;
    UntitledMultiSelectEnumSchema_type type;
};

/**
 * Use {@link TitledSingleSelectEnumSchema} instead.
 * This interface will be removed in a future version.
 */
struct LegacyTitledEnumSchema final {
    std::string LegacyTitledEnumSchema_default;
    std::string description;
    std::vector<std::string> LegacyTitledEnumSchema_enum;
    /**
     * (Legacy) Display names for enum values.
     * Non-standard according to JSON schema 2020-12.
     */
    std::vector<std::string> enumNames;
    std::string title;
    items_type type;
};

using default_union = std::variant<std::vector<std::string>, std::string>;

enum class EnumSchema_type : int { array, string };

/**
 * Schema for single-selection enumeration without display titles for options.
 *
 * Schema for single-selection enumeration with display titles for each option.
 *
 * Schema for multiple-selection enumeration without display titles for options.
 *
 * Schema for multiple-selection enumeration with display titles for each option.
 *
 * Use {@link TitledSingleSelectEnumSchema} instead.
 * This interface will be removed in a future version.
 */
struct EnumSchema final {
    /**
     * Optional default value.
     */
    default_union EnumSchema_default;
    /**
     * Optional description for the enum field.
     */
    std::string description;
    /**
     * Array of enum values to choose from.
     */
    std::vector<std::string> EnumSchema_enum;
    /**
     * Optional title for the enum field.
     */
    std::string title;
    EnumSchema_type type;
    /**
     * Array of enum options with values and display labels.
     */
    std::vector<oneOf> oneOf;
    /**
     * Schema for the array items.
     *
     * Schema for array items with enum options and display labels.
     */
    PrimitiveSchemaDefinition_items items;
    /**
     * Maximum number of items to select.
     */
    double maxItems;
    /**
     * Minimum number of items to select.
     */
    double minItems;
    /**
     * (Legacy) Display names for enum values.
     * Non-standard according to JSON schema 2020-12.
     */
    std::vector<std::string> enumNames;
};

/**
 * The result returned by the client for an {@link ElicitRequestelicitation/create} request.
 */
struct ElicitResult final {
    /**
     * The user action in response to the elicitation.
     * - `"accept"`: User submitted the form/confirmed the action
     * - `"decline"`: User explicitly declined the action
     * - `"cancel"`: User dismissed without making an explicit choice
     */
    action action;
    /**
     * The submitted form data, only present when action is `"accept"` and mode was `"form"`.
     * Contains values matching the requested schema.
     * Omitted for out-of-band mode responses.
     */
    std::map<std::string, default_value> content;
};

enum class ClientRequest_method : int { completioncomplete, promptsget, promptslist, resourceslist, resourcesread, resourcestemplateslist, serverdiscover, subscriptionslisten, toolscall, toolslist };

/**
 * Common params for any request.
 *
 * Common params for paginated requests.
 *
 * Parameters for a `resources/read` request.
 *
 * Parameters for a {@link SubscriptionsListenRequestsubscriptions/listen} request.
 *
 * Parameters for a `prompts/get` request.
 *
 * Parameters for a `tools/call` request.
 *
 * Parameters for a `completion/complete` request.
 */
struct request_params final {
    RequestMetaObject _meta;
    /**
     * An opaque token representing the current pagination position.
     * If provided, the server should return results starting after this cursor.
     */
    std::string cursor;
    /**
     * A map of client responses to server-initiated requests.
     * Keys correspond to the keys in the {@link InputRequests} map;
     * values are the client's result for each request.
     */
    std::map<std::string, InputResponse> inputResponses;
    std::string requestState;
    /**
     * The URI of the resource. The URI can use any protocol; it is up to the server how to
     * interpret it.
     */
    std::string uri;
    /**
     * The notifications the client opts in to on this stream. The server
     * **MUST NOT** send notification types the client has not explicitly
     * requested.
     */
    SubscriptionFilter notifications;
    /**
     * Arguments to use for templating the prompt.
     *
     * Arguments to use for the tool call.
     */
    std::map<std::string, JsonValue> arguments;
    /**
     * The name of the prompt or prompt template.
     *
     * The name of the tool.
     */
    std::string name;
    /**
     * The argument's information
     */
    argument argument;
    /**
     * Additional, optional context for completions
     */
    context context;
    ref ref;
};

/**
 * A request from the client asking the server to advertise its supported
 * protocol versions, capabilities, and other metadata. Servers **MUST**
 * implement `server/discover`. Clients **MAY** call it but are not required
 * to 
 version negotiation can also happen inline via per-request `_meta`.
 *
 * Sent from the client to request a list of resources the server has.
 *
 * Sent from the client to request a list of resource templates the server has.
 *
 * Sent from the client to the server, to read a specific resource URI.
 *
 * Sent from the client to open a long-lived channel for receiving notifications
 * outside the context of a specific request. Replaces the previous HTTP GET
 * endpoint and ensures consistent behavior between HTTP and STDIO.
 *
 * Sent from the client to request a list of prompts and prompt templates the server has.
 *
 * Used by the client to get a prompt provided by the server.
 *
 * Sent from the client to request a list of tools the server has.
 *
 * Used by the client to invoke a tool provided by the server.
 *
 * A request from the client to the server, to ask for completion options.
 */
struct ClientRequest final {
    RequestId id;
    jsonrpc jsonrpc;
    ClientRequest_method method;
    request_params params;
};

enum class ServerNotification_method : int { notificationscancelled, notificationsmessage, notificationsprogress, notificationspromptslist_changed, notificationsresourceslist_changed, notificationsresourcesupdated, notificationssubscriptionsacknowledged, notificationstoolslist_changed };

/**
 * Parameters for a `notifications/cancelled` notification.
 *
 * Parameters for a {@link ProgressNotificationnotifications/progress} notification.
 *
 * Common params for any notification.
 *
 * Parameters for a {@link
 * SubscriptionsAcknowledgedNotificationnotifications/subscriptions/acknowledged}
 * notification.
 *
 * Parameters for a `notifications/resources/updated` notification.
 *
 * Parameters for a `notifications/message` notification.
 */
struct notification_params final {
    /**
     * Extends {@link MetaObject} with additional notification-specific fields. All key naming
     * rules from `MetaObject` apply.
     */
    NotificationMetaObject _meta;
    /**
     * An optional string describing the reason for the cancellation. This MAY be logged or
     * presented to the user.
     */
    std::string reason;
    /**
     * The ID of the request to cancel.
     *
     * This MUST correspond to the ID of a request the client previously issued.
     */
    RequestId requestId;
    /**
     * An optional message describing the current progress.
     */
    std::string message;
    /**
     * The progress thus far. This should increase every time progress is made, even if the
     * total is unknown.
     */
    double progress;
    /**
     * The progress token which was given in the initial request, used to associate this
     * notification with the request that is proceeding.
     */
    RequestId progressToken;
    /**
     * Total number of items to process (or total progress required), if known.
     */
    double total;
    /**
     * The subset of requested notification types the server agreed to honor.
     * Only includes notification types the server actually supports; if the
     * client requested an unsupported type (e.g., `promptsListChanged` when
     * the server has no prompts), it is omitted from this set.
     */
    SubscriptionFilter notifications;
    /**
     * The URI of the resource that has been updated. This might be a sub-resource of the one
     * that the client actually subscribed to.
     */
    std::string uri;
    /**
     * The data to be logged, such as a string message or an object. Any JSON serializable type
     * is allowed here.
     */
    JsonValue data;
    /**
     * The severity of this log message.
     */
    LoggingLevel level;
    /**
     * An optional name of the logger issuing this message.
     */
    std::string logger;
};

/**
 * This notification is sent by the client to indicate that it is cancelling a request it
 * previously issued.
 *
 * On stdio, the server also sends this notification, solely to terminate a {@link
 * SubscriptionsListenRequestsubscriptions/listen} stream: it references the ID of the
 * `subscriptions/listen` request that opened the stream. Servers MUST NOT use this
 * notification to cancel any other request.
 *
 * The request SHOULD still be in-flight, but due to communication latency, it is always
 * possible that this notification MAY arrive after the request has already finished.
 *
 * This notification indicates that the result will be unused, so any associated processing
 * SHOULD cease.
 *
 * An out-of-band notification used to inform the receiver of a progress update for a
 * long-running request.
 *
 * An optional notification from the server to the client, informing it that the list of
 * resources it can read from has changed. This is only delivered on a {@link
 * SubscriptionsListenRequestsubscriptions/listen} stream when the client requested it via
 * the `resourcesListChanged` filter field.
 *
 * Sent by the server to acknowledge that a
 * {@link SubscriptionsListenRequestsubscriptions/listen} subscription has been
 * established and to report which notification types it agreed to honor.
 *
 * This notification MUST be the first message the server sends carrying the
 * subscription's ID in `io.modelcontextprotocol/subscriptionId`. The server MUST
 * NOT send any notification on the subscription before acknowledging it. On
 * stdio, where every subscription shares one channel, this ordering is defined
 * per subscription ID and not per channel: messages belonging to other
 * subscriptions MAY be interleaved before it.
 *
 * A notification from the server to the client, informing it that a resource has changed
 * and may need to be read again. This is only sent for resources the client opted in to via
 * the `resourceSubscriptions` field of a {@link
 * SubscriptionsListenRequestsubscriptions/listen} request.
 *
 * An optional notification from the server to the client, informing it that the list of
 * prompts it offers has changed. This is only delivered on a {@link
 * SubscriptionsListenRequestsubscriptions/listen} stream when the client requested it via
 * the `promptsListChanged` filter field.
 *
 * An optional notification from the server to the client, informing it that the list of
 * tools it offers has changed. This is only delivered on a {@link
 * SubscriptionsListenRequestsubscriptions/listen} stream when the client requested it via
 * the `toolsListChanged` filter field.
 *
 * JSONRPCNotification of a log message passed from server to client. The client opts in by
 * setting `"io.modelcontextprotocol/logLevel"` in a request's `_meta`.
 */
struct ServerNotification final {
    jsonrpc jsonrpc;
    ServerNotification_method method;
    /**
     * Common params for any notification.
     */
    notification_params params;
};

/**
 * Extends {@link MetaObject} with additional result-specific fields. All key naming rules
 * from `MetaObject` apply.
 *
 * Extends {@link ResultMetaObject} with the subscription-stream identifier carried by a
 * {@link SubscriptionsListenResult}. All key naming rules from `MetaObject` apply.
 */
struct result_meta_object final {
    /**
     * Identifies the server software producing the response. Servers SHOULD
     * include this field on every response unless specifically configured not
     * to do so.
     *
     * The {@link Implementation} schema requires `name` and `version`; other
     * fields are optional.
     *
     * The value is self-reported by the server and is not verified by the
     * protocol. It is intended for display, logging, and debugging. Clients
     * SHOULD NOT use it to change their behavior, and SHOULD NOT rely on it for
     * security decisions.
     */
    Implementation iomodelcontextprotocolserverInfo;
    /**
     * Identifies the subscription stream this response closes, so the client can
     * correlate it with the originating subscription 
 mirroring the same key on
     * the stream's notifications. The value is the JSON-RPC ID of the
     * `subscriptions/listen` request that opened the stream (and equals this
     * response's `id`).
     */
    RequestId iomodelcontextprotocolsubscriptionId;
};

/**
 * Common result fields.
 *
 * An InputRequiredResult sent by the server to indicate that additional input is needed
 * before the request can be completed.
 *
 * At least one of `inputRequests` or `requestState` MUST be present.
 *
 * The result returned by the server for a {@link DiscoverRequestserver/discover} request.
 *
 * The result returned by the server for a {@link ListResourcesRequestresources/list}
 * request.
 *
 * The result returned by the server for a {@link
 * ListResourceTemplatesRequestresources/templates/list} request.
 *
 * The result returned by the server for a {@link ReadResourceRequestresources/read}
 * request.
 *
 * The response to a {@link SubscriptionsListenRequestsubscriptions/listen}
 * request, signalling that the subscription has ended gracefully (for example,
 * during server shutdown). Because the listen stream is long-lived, this result
 * is sent only when the server tears the subscription down; an abrupt transport
 * close carries no response. The result body is otherwise empty.
 *
 * The result returned by the server for a {@link ListPromptsRequestprompts/list} request.
 *
 * The result returned by the server for a {@link GetPromptRequestprompts/get} request.
 *
 * The result returned by the server for a {@link ListToolsRequesttools/list} request.
 *
 * The result returned by the server for a {@link CallToolRequesttools/call} request.
 *
 * The result returned by the server for a {@link CompleteRequestcompletion/complete}
 * request.
 */
struct ServerResult final {
    /**
     * Extends {@link MetaObject} with additional result-specific fields. All key naming rules
     * from `MetaObject` apply.
     */
    result_meta_object _meta;
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    std::string resultType;
    /**
     * A map of server-initiated requests that the client must fulfill.
     * Keys are server-assigned identifiers; values are the request objects.
     */
    std::map<std::string, InputRequest> inputRequests;
    std::string requestState;
    /**
     * Indicates the intended scope of the cached response, analogous to HTTP
     * `Cache-Control: public` vs `Cache-Control: private`.
     *
     * - `"public"`: The response does not contain user-specific data. Any
     * client or intermediary (e.g., shared gateway, caching proxy) MAY cache
     * the response and serve it across authorization contexts.
     * - `"private"`: The response MAY be cached and reused only within the
     * same authorization context. Caches MUST NOT be shared across
     * authorization contexts (e.g., a different access token requires a
     * different cache).
     */
    cacheScope cacheScope;
    /**
     * The capabilities of the server.
     */
    ServerCapabilities capabilities;
    /**
     * Natural-language guidance describing the server and its features.
     *
     * This can be used by clients to improve an LLM's understanding of
     * available tools (e.g., by including it in a system prompt). It should
     * focus on information that helps the model use the server effectively
     * and should not duplicate information already in tool descriptions.
     */
    std::string instructions;
    /**
     * MCP Protocol Versions this server supports. The client should choose a
     * version from this list for use in subsequent requests.
     */
    std::vector<std::string> supportedVersions;
    /**
     * A hint from the server indicating how long (in milliseconds) the
     * client MAY cache this response before re-fetching. Semantics are
     * analogous to HTTP Cache-Control max-age.
     *
     * - If 0, The response SHOULD be considered immediately stale,
     * The client MAY re-fetch every time the result is needed.
     * - If positive, the client SHOULD consider the result fresh for this many
     * milliseconds after receiving the response.
     */
    double ttlMs;
    /**
     * An opaque token representing the pagination position after the last returned result.
     * If present, there may be more results available.
     */
    std::string nextCursor;
    std::vector<Resource> resources;
    std::vector<ResourceTemplate> resourceTemplates;
    std::vector<resource> contents;
    std::vector<Prompt> prompts;
    /**
     * An optional description for the prompt.
     */
    std::string description;
    std::vector<PromptMessage> messages;
    std::vector<Tool> tools;
    /**
     * A list of content objects that represent the unstructured result of the tool call.
     */
    std::vector<ContentBlock> content;
    /**
     * Whether the tool call ended in an error.
     *
     * If not set, this is assumed to be false (the call was successful).
     *
     * Any errors that originate from the tool SHOULD be reported inside the result
     * object, with `isError` set to true, _not_ as an MCP protocol-level error
     * response. Otherwise, the LLM would not be able to see that an error occurred
     * and self-correct.
     *
     * However, any errors in _finding_ the tool, an error indicating that the
     * server does not support tool calls, or any other exceptional conditions,
     * should be reported as an MCP error response.
     */
    bool isError;
    /**
     * An optional JSON value that represents the structured result of the tool call.
     *
     * This can be any JSON value (object, array, string, number, boolean, or null)
     * that conforms to the tool's outputSchema if one is defined.
     */
    JsonValue structuredContent;
    completion completion;
};

} // namespace mcp::v2026_07_28
