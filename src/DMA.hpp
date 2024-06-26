#include "plugin.hpp"
#include "Utility.hpp"
#include <set>

namespace sparkette {

	template <typename T>
	class DMAChannel;

	template <typename T>
	struct DMAWriteEvent {
		DMAChannel<T> *channel;
		std::size_t index;
	};

	template <typename T>
	class DMAHost;
	
	template <typename T>
	class DMAChannel {
	protected:
		T *mem_start = nullptr;
		DMAHost<T> *owner = nullptr;
		std::size_t count = 0;
		std::size_t stride = 1;
		std::size_t columns = 1;

		void signalDMAWrite(std::size_t index) {
			if (owner) {
				DMAWriteEvent<T> e;
				e.channel = this;
				e.index = index;
				owner->onDMAWrite(e);
			}
		}

	public:
		class accessor {
			friend class DMAChannel<T>;
			DMAChannel *channel;
			std::size_t index;
		public:
			operator T() const { return channel->read(index); }
			accessor& operator=(T rhs) {
				channel->write(index, rhs);
				return *this;
			}
		};

		void setup(DMAHost<T> *owner, std::size_t count, T *mem_start = nullptr, std::size_t stride = 1) {
			this->owner = owner;
			this->count = count;
			this->mem_start = mem_start;
			this->stride = stride;
			columns = 1;
		}

		void setup(DMAHost<T> *owner, std::size_t width, std::size_t height, T *mem_start = nullptr, std::size_t stride = 1) {
			setup(owner, width * height, mem_start, stride);
			columns = width;
		}

		DMAChannel() = default;
		DMAChannel(DMAHost<T> *owner, T *mem_start, std::size_t count, std::size_t columns = 1, std::size_t stride = 1)
			: owner(owner), mem_start(mem_start), count(count), stride(stride), columns(columns) {}
		DMAChannel(const DMAChannel& other) = delete;
		DMAChannel& operator=(const DMAChannel& other) = delete;

		virtual T read(std::size_t index) const {
			return mem_start[index * stride];
		}

		T read(std::size_t col, std::size_t row) const {
			return read(columns * row + col);
		}

		virtual void write(std::size_t index, T value) {
			mem_start[index * stride] = value;
			signalDMAWrite(index);
		}

		void write(std::size_t col, std::size_t row, T value) {
			write(columns * row + col, value);
		}

		std::size_t size() const {
			return count;
		}
		
		std::size_t width() const {
			if (columns == 0)
				return count;
			else
				return columns;
		}

		std::size_t height() const {
			if (columns == 0)
				return 1;
			else
				return count / columns;
		}

		DMAHost<T> *getOwner() const {
			return owner;
		}

		accessor operator[](std::size_t index) {
			accessor acc;
			acc.channel = this;
			acc.index = index;
			return acc;
		}

		T operator[](std::size_t index) const {
			return read(index);
		}
	};

	template <typename T>
	struct DMAHost {
		virtual int getDMAChannelCount() const = 0;
		virtual DMAChannel<T> *getDMAChannel(int num) = 0;
		virtual const DMAChannel<T> *getDMAChannel(int num) const {
			auto this_nonconst = const_cast<DMAHost*>(this);
			return const_cast<const DMAChannel<T>*>(this_nonconst->getDMAChannel(num));
		}
		virtual bool readyForDMA() const {
			return true;
		}
		virtual void onDMAWrite(const DMAWriteEvent<T> &e) {}
	};

	template <typename T>
	class DMAClient : public DMAHost<T> {
		DMAHost<T> *host = nullptr;

	public:
		struct DMAHostChangeEvent {
			DMAHost<T> *oldHost;
			DMAHost<T> *newHost;
		};

	protected:
		void setDMAHost(DMAHost<T> *newHost) {
			DMAHostChangeEvent e;
			e.oldHost = host;
			host = e.newHost = newHost;
			onDMAHostChange(e);
		}

	public:
		virtual void onDMAHostChange(const DMAHostChangeEvent &e) {}

		DMAHost<T> *getDMAHost() const {
			return host;
		}

		int getDMAChannelCount() const override {
			if (host)
				return host->getDMAChannelCount();
			else
				return 0;
		}

		DMAChannel<T> *getDMAChannel(int num) override {
			if (host)
				return host->getDMAChannel(num);
			else
				return nullptr;
		}

		const DMAChannel<T> *getDMAChannel(int num) const override {
			if (host)
				return host->getDMAChannel(num);
			else
				return nullptr;
		}

		bool readyForDMA() const override {
			if (host)
				return host->readyForDMA();
			else
				return false;
		}
	};

	template <typename TFirst, typename... TRest>
	inline bool checkForDMAClient(Module *module) {
		bool result = dynamic_cast<DMAClient<TFirst>*>(module) != nullptr;
		if constexpr (sizeof...(TRest) > 0)
			return result || checkForDMAClient<TRest...>(module);
		else
			return result;
	}

	template <typename TModule, template <typename> typename TTemplate, typename TFirst, typename... TRest>
	inline int checkMultiDMAChannels(const TModule *module, int scratch = 0) {
		int n = std::max(scratch, module->TTemplate<TFirst>::getDMAChannelCount());
		if constexpr (sizeof...(TRest) == 0)
			return n;
		else
			return checkMultiDMAChannels<TModule, TTemplate, TRest...>(module, n);
	}

	template <typename... T>
	class DMAHostModule : public Module, public DMAHost<T>... {
	protected:
		int dmaClientLightID = -1;
	
	public:
		virtual void onExpanderChange(const ExpanderChangeEvent &e) override {
			if (e.side == 0 && dmaClientLightID >= 0)
				lights[dmaClientLightID].setBrightness(checkForDMAClient<T...>(leftExpander.module) ? 1.f : 0.f);
		}

		int getDMAChannelCount() {
			return checkMultiDMAChannels<DMAHostModule<T...>, DMAHost, T...>(this);
		}
	};

	template <typename... T>
	class DMAExpanderModule : public Module, public DMAClient<T>... {
		template <typename TFirst, typename... TRest>
		void setDMAHosts(Module *module) {
			DMAClient<TFirst>::setDMAHost(dynamic_cast<DMAHost<TFirst>*>(module));
			if constexpr (sizeof...(TRest) > 0)
				setDMAHosts<TRest...>(module);
		}

		template <typename TFirst, typename... TRest>
		bool checkHostReady(bool &hostFound) const {
			DMAHost<TFirst> *host = DMAClient<TFirst>::getDMAHost();
			if (host) {
				hostFound = true;
				if (host->readyForDMA())
					return true;
				else
					goto next_host_type;
			} else {
			next_host_type:
				if constexpr (sizeof...(TRest) > 0)
					return checkHostReady<TRest...>(hostFound);
			}
			return false;
		}

	protected:
		int dmaHostLightID = -1; //intended to be a GreenRedLight, so dmaHostLightID+1 will be used as well
		int dmaClientLightID = -1;
		
		bool isHostReady() const {
			bool _;
			return checkHostReady<T...>(_);
		}

		bool isHostReady(bool &hostFound) const {
			return checkHostReady<T...>(hostFound);
		}
		
	public:
		virtual void onExpanderChange(const ExpanderChangeEvent &e) override {
			if (e.side == 0) {
				if (dmaClientLightID >= 0)
					lights[dmaClientLightID].setBrightness(checkForDMAClient<T...>(leftExpander.module) ? 1.f : 0.f);
			} else {
				setDMAHosts<T...>(rightExpander.module);
			}
		}

		int getDMAChannelCount() /* NOT const, NOT override */ {
			return checkMultiDMAChannels<DMAExpanderModule<T...>, DMAClient, T...>(this);
		}

		virtual void process(const ProcessArgs &args) override {
			if (dmaHostLightID >= 0) {
				bool hostFound = false;
				bool ready = checkHostReady<T...>(hostFound);
				if (hostFound) {
					lights[dmaHostLightID].setBrightnessSmooth(ready ? 1.f : 0.f, args.sampleTime);
					lights[dmaHostLightID+1].setBrightnessSmooth(ready ? 0.f : 1.f, args.sampleTime);
				} else {
					lights[dmaHostLightID].setBrightnessSmooth(0.f, args.sampleTime);
					lights[dmaHostLightID+1].setBrightnessSmooth(0.f, args.sampleTime);
				}
			}
		}
	};
}
